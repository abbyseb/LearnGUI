# workers.py - Pure Python pipeline workers (no MATLAB)
from __future__ import annotations
from concurrent.futures import ThreadPoolExecutor, wait, FIRST_COMPLETED, as_completed
from pathlib import Path
from typing import List, Optional, Tuple
import os
import sys
import shutil
from PyQt6.QtCore import QThread, pyqtSignal
import numpy as np

# Ensure project root (parent of gui/) is in path so workers can import modules.*
_PROJECT_ROOT = Path(__file__).resolve().parent.parent
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))

try:
    import pydicom
except ImportError:
    pydicom = None

# Eagerly load ITK-RTK on main thread (avoids import issues when DRR worker runs in QThread)
try:
    import itk
    _ = itk.RTK
except Exception:
    pass


def _copy_pool_workers(requested: int, n_jobs: int) -> int:
    """Clamp copy thread count (high values → UI freezes + SMB thrash). Override cap with LEARNGUI_COPY_WORKERS_MAX."""
    try:
        cap = int(os.environ.get("LEARNGUI_COPY_WORKERS_MAX", "32").strip() or "32")
    except ValueError:
        cap = 32
    cap = max(2, min(cap, 96))
    req = max(1, requested)
    if n_jobs <= 0:
        return min(req, cap)
    return max(1, min(req, cap, n_jobs))


class BasePythonWorker(QThread):
    finished_ok = pyqtSignal(str)
    failed = pyqtSignal(str)
    line_received = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._cancelled = False

    def log(self, msg: str):
        self.line_received.emit(msg)

    def cancel(self):
        self._cancelled = True


class CopyWorker(BasePythonWorker):
    """Emits file_progress(done, total) as files are copied (total = pending file copy count)."""

    file_progress = pyqtSignal(int, int)

    def __init__(self, patient_id: str, rt_list: list, src_base: str, dst_base: Path, parent=None):
        super().__init__(parent)
        self.patient_id = patient_id
        self.rt_list = rt_list
        self.src_base = Path(src_base)
        self.dst_base = Path(dst_base)

    def run(self):
        try:
            from .run_preparation import build_train_pairs_for_run
            pairs = build_train_pairs_for_run(self.rt_list, self.dst_base, str(self.src_base))
            self.log(f"COPY: {len(pairs)} source(s)")

            jobs: List[Tuple[Path, Path]] = []
            for src, dst in pairs:
                if self._cancelled:
                    break
                if not src.exists():
                    self.log(f"ERROR: Source not found: {src}")
                    continue
                dst.mkdir(parents=True, exist_ok=True)
                for root, _, files in os.walk(src):
                    if self._cancelled:
                        break
                    rel = Path(root).relative_to(src)
                    (dst / rel).mkdir(parents=True, exist_ok=True)
                    for f in files:
                        if self._cancelled:
                            break
                        sf, df = Path(root) / f, dst / rel / f
                        if not df.exists():
                            jobs.append((sf, df))

            try:
                n_workers = int(os.environ.get("LEARNGUI_COPY_WORKERS", "8").strip() or "8")
            except ValueError:
                n_workers = 8
            n_workers = _copy_pool_workers(n_workers, len(jobs))

            use_copyfile = os.environ.get("LEARNGUI_COPY_USE_COPYFILE", "").strip().lower() in (
                "1",
                "true",
                "yes",
            )

            def copy_one(pair: Tuple[Path, Path]) -> int:
                if self._cancelled:
                    return 0
                sf, df = pair
                try:
                    df.parent.mkdir(parents=True, exist_ok=True)
                    if use_copyfile:
                        shutil.copyfile(sf, df)
                        try:
                            shutil.copystat(sf, df, follow_symlinks=False)
                        except OSError:
                            pass
                    else:
                        shutil.copy2(sf, df)
                    return 1
                except Exception as e:
                    self.log(f"ERROR: copy {sf} -> {df}: {e}")
                    return 0

            total = 0
            if jobs:
                n_jobs = len(jobs)
                self.file_progress.emit(0, n_jobs)
                self.log(
                    f"COPY: copying {n_jobs} file(s) with {n_workers} parallel worker(s); "
                    f"tune with LEARNGUI_COPY_WORKERS / LEARNGUI_COPY_WORKERS_MAX; "
                    f"LEARNGUI_COPY_USE_COPYFILE=1 can help on some network shares."
                )
                if use_copyfile:
                    self.log("COPY: using copyfile+copystat (LEARNGUI_COPY_USE_COPYFILE).")
                report_every = max(1, min(200, n_jobs // 40 or 1))
                done_files = 0
                with ThreadPoolExecutor(max_workers=n_workers) as pool:
                    futures = [pool.submit(copy_one, j) for j in jobs]
                    for fut in as_completed(futures):
                        total += fut.result()
                        done_files += 1
                        if self._cancelled:
                            break
                        if (
                            n_jobs <= 200
                            or done_files == 1
                            or done_files == n_jobs
                            or done_files % report_every == 0
                        ):
                            self.file_progress.emit(done_files, n_jobs)
                        if (
                            done_files == 1
                            or done_files == n_jobs
                            or done_files % report_every == 0
                        ):
                            self.log(f"COPY progress: {done_files}/{n_jobs}")
                    if not self._cancelled and n_jobs > 0:
                        self.file_progress.emit(n_jobs, n_jobs)
            else:
                self.log("COPY: no files to copy (already up to date)")
                self.file_progress.emit(0, 0)

            self.log(f"COPY complete: {total} files")
            self.finished_ok.emit("COPY")
        except Exception as e:
            self.failed.emit(str(e))


class Dicom2MhaWorker(BasePythonWorker):
    def __init__(self, train_dir: Path, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)

    def run(self):
        try:
            self.log("DICOM2MHA: Starting conversion thread...")
            self.log(f"DICOM2MHA: Input/output directory: {self.train_dir}")
            from modules.dicom2mha.convert import convert_dicom_to_mha

            def on_status(msg: str):
                self.log(f"DICOM2MHA: {msg}")

            def on_phase_done(done: int, total: int, filename: str):
                self.log(f"DICOM2MHA: {filename} done ({done}/{total})")

            convert_dicom_to_mha(
                self.train_dir, self.train_dir,
                on_phase_done=on_phase_done, on_status=on_status
            )
            self.log("DICOM2MHA: Complete")
            self.finished_ok.emit("DICOM2MHA")
        except Exception as e:
            self.failed.emit(str(e))


class DownsampleWorker(BasePythonWorker):
    def __init__(self, train_dir: Path, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)

    def run(self):
        try:
            self.log(f"DOWNSAMPLE: Processing {self.train_dir}")
            from modules.downsampling.downsample import process_directory

            def on_volume_done(done: int, total: int, filename: str):
                self.log(f"DOWNSAMPLE: {filename} done ({done}/{total})")

            process_directory(self.train_dir, on_volume_done=on_volume_done)
            self.log("DOWNSAMPLE: Complete")
            self.finished_ok.emit("DOWNSAMPLE")
        except Exception as e:
            self.failed.emit(str(e))


class DrrWorker(BasePythonWorker):
    def __init__(
        self,
        train_dir: Path,
        geom_path: Path,
        drr_opts: dict | None = None,
        parent=None,
    ):
        super().__init__(parent)
        self.train_dir = Path(train_dir)
        self.geom_path = Path(geom_path)
        self.drr_opts = dict(drr_opts or {})

    def run(self):
        try:
            import re

            from modules.drr_generation.generate import generate_drrs

            geom = str(self.geom_path)
            opts = dict(self.drr_opts)
            opts["geometry_path"] = self.geom_path
            self.log(
                f"DRR: forward_project pipeline → {self.train_dir} (geometry: {geom})"
            )

            mhas = sorted(self.train_dir.glob("CT_*.mha"))
            if not mhas:
                mhas = sorted(self.train_dir.glob("sub_CT_*.mha"))
            if not mhas:
                self.failed.emit("DRR: No CT_*.mha or sub_CT_*.mha in train/")
                return

            n = len(mhas)
            for i, mha in enumerate(mhas, start=1):
                if self._cancelled:
                    break
                m = re.search(r"(\d+)", mha.stem)
                ct_num = int(m.group(1)) if m else i
                self.log(f"DRR: ({i}/{n}) {mha.name}")
                ok, err = generate_drrs(
                    str(mha),
                    str(self.train_dir),
                    geometry_file=geom,
                    ct_num=ct_num,
                    **opts,
                )
                if not ok:
                    self.log(f"DRR ERROR: {err}")
                    # Step manager prefixes with "DRR: "; avoid "DRR: DRR: …"
                    self.failed.emit(f"generate_drrs failed for {mha.name}: {err}")
                    return

            self.log("DRR: Complete")
            self.finished_ok.emit("DRR")
        except Exception as e:
            self.failed.emit(str(e))


class CompressWorker(BasePythonWorker):
    def __init__(self, train_dir: Path, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)

    def run(self):
        try:
            self.log("COMPRESS: Converting projections to binary")
            from modules.drr_compression.compress import process_directory

            def on_batch_done(done: int, total: int):
                self.log(f"COMPRESS: {done}/{total} projections")

            process_directory(self.train_dir, on_batch_done=on_batch_done)
            self.log("COMPRESS: Complete")
            self.finished_ok.emit("COMPRESS")
        except Exception as e:
            self.failed.emit(str(e))


class Dvf2DWorker(BasePythonWorker):
    """Placeholder - 2D DVF generation (complex, may need external tool)."""
    def __init__(self, train_dir: Path, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)

    def run(self):
        self.log("DVF2D: Not implemented in pure Python (requires MATLAB/external)")
        self.finished_ok.emit("DVF2D")


class Dvf3DWorker(BasePythonWorker):
    def __init__(self, train_dir: Path, low_res: bool, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)
        self.low_res = low_res

    def run(self):
        try:
            mode = "downsampled" if self.low_res else "full-res"
            step_id = "DVF3D_LOW" if self.low_res else "DVF3D_FULL"
            self.log(f"DVF3D ({mode}): Starting from {self.train_dir}")
            from modules.dvf_generation.generate import generate_dvf
            mod_path = Path(__file__).resolve().parent.parent / "modules" / "dvf_generation"
            param_file = mod_path / "Elastix_BSpline_Sliding.txt"
            fixed = self.train_dir / "sub_CT_06.mha" if self.low_res else self.train_dir / "CT_06.mha"
            moving_list = sorted(self.train_dir.glob("sub_CT_*.mha")) if self.low_res else sorted(self.train_dir.glob("CT_*.mha"))
            prefix = "DVF_sub_" if self.low_res else "DVF_"
            if not fixed.exists():
                self.log(f"DVF3D: Fixed reference {fixed.name} not found, skipping")
                self.finished_ok.emit(step_id)
                return
            if not param_file.exists():
                self.failed.emit(f"Elastix parameter file not found: {param_file}")
                return
            done = 0
            for m in moving_list:
                if self._cancelled:
                    break
                num = m.stem.replace("sub_CT_", "").replace("CT_", "").split(".")[0]
                if num == "06":
                    continue
                out = self.train_dir / f"{prefix}{num}.mha"
                if m.exists():
                    self.log(f"DVF3D: Registering {m.name} -> fixed (CT_06)")
                    generate_dvf(fixed, m, param_file, out)
                    done += 1
            self.log(f"DVF3D ({mode}): Complete ({done} DVFs)")
            self.finished_ok.emit(step_id)
        except Exception as e:
            self.failed.emit(str(e))


class KvPreprocessWorker(BasePythonWorker):
    def __init__(self, rt_list: list, base_dir: str, patient_root: Path, parent=None):
        super().__init__(parent)
        self.rt_list = rt_list
        self.base_dir = base_dir
        self.patient_root = Path(patient_root)

    def run(self):
        try:
            from modules.kv_preprocess.process import copy_test_data, process_kv_images
            if self.rt_list:
                copy_test_data(self.rt_list[0], self.base_dir, self.patient_root)
            test_dir = self.patient_root / "test"
            if test_dir.exists():
                process_kv_images(test_dir)
            # Copy source.mha (exhale reference)
            train = self.patient_root / "train"
            if not train.exists():
                train = self.patient_root / "train"
            ct06 = train / "CT_06.mha" if train.exists() else None
            if ct06 and ct06.exists():
                shutil.copy2(ct06, train / "source.mha")
            self.finished_ok.emit("KV_PREPROCESS")
        except Exception as e:
            self.failed.emit(str(e))


class PythonPipelineJob(QThread):
    """Runs pipeline steps sequentially using Python workers."""
    finished_ok = pyqtSignal(str)
    failed = pyqtSignal(str)
    line_received = pyqtSignal(str)
    step_complete = pyqtSignal(str)

    def __init__(self, rt_list: list, work_root: Path, train_dir: Path,
                 skip_copy: bool, skip_dicom2mha: bool, skip_downsample: bool,
                 skip_drr: bool, skip_compress: bool,
                 include_2d_dvf: bool, do_3d_low: bool, do_3d_full: bool,
                 skip_kv_preprocess: bool, base_dir: str, patient_id: str,
                 drr_generation_options: dict | None = None,
                 parent=None):
        super().__init__(parent)
        self.rt_list = rt_list
        self.work_root = Path(work_root)
        self.train_dir = Path(train_dir)
        self.base_dir = base_dir
        self.patient_id = patient_id
        self.skip_copy = skip_copy
        self.skip_dicom2mha = skip_dicom2mha
        self.skip_downsample = skip_downsample
        self.skip_drr = skip_drr
        self.skip_compress = skip_compress
        self.include_2d_dvf = include_2d_dvf
        self.do_3d_low = do_3d_low
        self.do_3d_full = do_3d_full
        self.skip_kv_preprocess = skip_kv_preprocess
        self.drr_generation_options = drr_generation_options or {}
        self._cancelled = False
        self._worker_failed = False

    def _log(self, msg: str):
        self.line_received.emit(msg)

    def run(self):
        geom_path = Path(__file__).parent.parent / "modules" / "drr_generation" / "RTKGeometry_360.xml"
        step_defs = [
            ("COPY", not self.skip_copy),
            ("DICOM2MHA", not self.skip_dicom2mha),
            ("DOWNSAMPLE", not self.skip_downsample),
            ("DRR", not self.skip_drr),
            ("COMPRESS", not self.skip_compress),
            ("DVF2D", self.include_2d_dvf),
            ("DVF3D_LOW", self.do_3d_low),
            ("DVF3D_FULL", self.do_3d_full),
            ("KV_PREPROCESS", not self.skip_kv_preprocess),
        ]
        for name, run_it in step_defs:
            if self._cancelled:
                break
            if not run_it:
                self._log(f"Skipping {name} (not selected)")
                continue
            self._log(f"Running step: {name}")
            try:
                self._run_step(name, geom_path)
                self.step_complete.emit(name)
            except Exception as e:
                self._log(f"ERROR: {name} failed: {e}")
                self.failed.emit(f"{name}: {e}")
                return
            if self._cancelled:
                break
        self.finished_ok.emit("Pipeline complete")

    def _run_step(self, name: str, geom_path: Path):
        """Execute one pipeline step directly on this thread (no nested QThread)."""
        if name == "COPY":
            self._run_copy()
        elif name == "DICOM2MHA":
            self._run_dicom2mha()
        elif name == "DOWNSAMPLE":
            self._run_downsample()
        elif name == "DRR":
            self._run_drr(geom_path)
        elif name == "COMPRESS":
            self._run_compress()
        elif name == "DVF2D":
            self._log("DVF2D: Not implemented in pure Python (requires MATLAB/external)")
        elif name == "DVF3D_LOW":
            self._run_dvf3d(low_res=True)
        elif name == "DVF3D_FULL":
            self._run_dvf3d(low_res=False)
        elif name == "KV_PREPROCESS":
            self._run_kv_preprocess()

    def _run_copy(self):
        from .run_preparation import build_train_pairs_for_run
        # train_dir is …/<Patient>/train → run folder is two levels up
        run_folder = self.train_dir.parent.parent
        pairs = build_train_pairs_for_run(self.rt_list, run_folder, self.base_dir)
        self._log(f"COPY: {len(pairs)} source(s)")
        jobs: List[Tuple[Path, Path]] = []
        for src, dst in pairs:
            if self._cancelled:
                break
            if not src.exists():
                self._log(f"ERROR: Source not found: {src}")
                continue
            dst.mkdir(parents=True, exist_ok=True)
            for root, _, files in os.walk(src):
                if self._cancelled:
                    break
                rel = Path(root).relative_to(src)
                (dst / rel).mkdir(parents=True, exist_ok=True)
                for f in files:
                    if self._cancelled:
                        break
                    sf, df = Path(root) / f, dst / rel / f
                    if not df.exists():
                        jobs.append((sf, df))
        try:
            n_workers = int(os.environ.get("LEARNGUI_COPY_WORKERS", "8").strip() or "8")
        except ValueError:
            n_workers = 8
        n_workers = _copy_pool_workers(n_workers, len(jobs))
        log_fn = self._log
        cancelled = lambda: self._cancelled

        use_copyfile = os.environ.get("LEARNGUI_COPY_USE_COPYFILE", "").strip().lower() in (
            "1",
            "true",
            "yes",
        )

        def copy_one(pair: Tuple[Path, Path]) -> int:
            if cancelled():
                return 0
            sf, df = pair
            try:
                df.parent.mkdir(parents=True, exist_ok=True)
                if use_copyfile:
                    shutil.copyfile(sf, df)
                    try:
                        shutil.copystat(sf, df, follow_symlinks=False)
                    except OSError:
                        pass
                else:
                    shutil.copy2(sf, df)
                return 1
            except Exception as e:
                log_fn(f"ERROR: copy {sf} -> {df}: {e}")
                return 0

        total = 0
        n_jobs = len(jobs)
        if jobs:
            try:
                stall_sec = float(
                    os.environ.get("LEARNGUI_COPY_STALL_LOG_SEC", "45").strip() or "45"
                )
            except ValueError:
                stall_sec = 45.0
            stall_sec = max(10.0, min(stall_sec, 600.0))

            self._log(f"COPY: copying {n_jobs} file(s) with {n_workers} worker(s)")
            if use_copyfile:
                self._log(
                    "COPY: using copyfile+copystat (LEARNGUI_COPY_USE_COPYFILE) — faster on some network disks."
                )
            report_every = max(1, min(200, n_jobs // 40 or 1))
            tail = min(24, max(5, n_jobs // 50))
            done_files = 0
            with ThreadPoolExecutor(max_workers=n_workers) as pool:
                pending = {pool.submit(copy_one, j) for j in jobs}
                while pending:
                    if self._cancelled:
                        break
                    done, still = wait(pending, timeout=stall_sec, return_when=FIRST_COMPLETED)
                    if not done:
                        self._log(
                            f"COPY: no file finished in {int(stall_sec)}s — "
                            f"{len(still)} still copying ({done_files}/{n_jobs} done). "
                            f"Often slow SMB/metadata; try LEARNGUI_COPY_WORKERS=2 or LEARNGUI_COPY_USE_COPYFILE=1."
                        )
                        continue
                    for fut in done:
                        pending.discard(fut)
                        total += fut.result()
                        done_files += 1
                        in_tail = done_files > n_jobs - tail and (
                            done_files % 3 == 0 or done_files == n_jobs
                        )
                        if (
                            done_files == 1
                            or done_files == n_jobs
                            or done_files % report_every == 0
                            or in_tail
                        ):
                            self._log(f"COPY progress: {done_files}/{n_jobs}")
        else:
            self._log("COPY: no files to copy (already up to date)")
            self._log("COPY progress: 1/1")
        self._log(f"COPY complete: {total} files")

    def _run_dicom2mha(self):
        self._log("DICOM2MHA: Starting conversion...")
        self._log(f"DICOM2MHA: Input/output directory: {self.train_dir}")
        from modules.dicom2mha.convert import convert_dicom_to_mha
        convert_dicom_to_mha(
            self.train_dir, self.train_dir,
            on_phase_done=lambda done, total, fn: self._log(f"DICOM2MHA: {fn} done ({done}/{total})"),
            on_status=lambda msg: self._log(f"DICOM2MHA: {msg}"),
        )
        self._log("DICOM2MHA: Complete")

    def _run_downsample(self):
        self._log(f"DOWNSAMPLE: Processing {self.train_dir}")
        from modules.downsampling.downsample import process_directory
        process_directory(
            self.train_dir,
            on_volume_done=lambda done, total, fn: self._log(f"DOWNSAMPLE: {fn} done ({done}/{total})"),
        )
        self._log("DOWNSAMPLE: Complete")

    def _run_drr(self, geom_path: Path):
        import re

        self._log(f"DRR: forward_project pipeline → {self.train_dir}")
        from modules.drr_generation.generate import generate_drrs

        opts = dict(self.drr_generation_options)
        opts["geometry_path"] = geom_path
        gs = opts.get("geometry_source", "xml")
        self._log(
            f"DRR: geometry_source={gs} xml={opts.get('geometry_path')} "
            f"detector_O={opts.get('detector_origin')} spacing={opts.get('detector_spacing')} "
            f"size_xy={opts.get('detector_size_xy')}"
        )
        if gs == "circular":
            self._log(
                f"DRR: circular SID={opts.get('circular_sid_mm')} SDD={opts.get('circular_sdd_mm')} "
                f"step°={opts.get('circular_angle_step_deg')} n={opts.get('circular_num_projections')}"
            )

        mhas = sorted(self.train_dir.glob("CT_*.mha"))
        if not mhas:
            mhas = sorted(self.train_dir.glob("sub_CT_*.mha"))
        if not mhas:
            raise RuntimeError("No CT_*.mha or sub_CT_*.mha in train/")
        n = len(mhas)
        for i, mha in enumerate(mhas, start=1):
            if self._cancelled:
                break
            m = re.search(r"(\d+)", mha.stem)
            ct_num = int(m.group(1)) if m else i
            self._log(f"DRR: ({i}/{n}) {mha.name}")
            ok, err = generate_drrs(
                str(mha),
                str(self.train_dir),
                geometry_file=str(geom_path),
                ct_num=ct_num,
                **opts,
            )
            if not ok:
                raise RuntimeError(err)
        self._log("DRR: Complete")

    def _run_compress(self):
        self._log("COMPRESS: Converting projections to binary")
        from modules.drr_compression.compress import process_directory
        process_directory(
            self.train_dir,
            on_batch_done=lambda done, total: self._log(f"COMPRESS: {done}/{total} projections"),
        )
        self._log("COMPRESS: Complete")

    def _run_dvf3d(self, low_res: bool):
        mode = "downsampled" if low_res else "full-res"
        self._log(f"DVF3D ({mode}): Starting from {self.train_dir}")
        from modules.dvf_generation.generate import generate_dvf
        mod_path = Path(__file__).resolve().parent.parent / "modules" / "dvf_generation"
        param_file = mod_path / "Elastix_BSpline_Sliding.txt"
        fixed = self.train_dir / ("sub_CT_06.mha" if low_res else "CT_06.mha")
        moving_list = sorted(self.train_dir.glob("sub_CT_*.mha" if low_res else "CT_*.mha"))
        prefix = "DVF_sub_" if low_res else "DVF_"
        if not fixed.exists():
            self._log(f"DVF3D: Fixed reference {fixed.name} not found, skipping")
            return
        if not param_file.exists():
            raise FileNotFoundError(f"Elastix parameter file not found: {param_file}")
        done = 0
        for m in moving_list:
            if self._cancelled:
                break
            num = m.stem.replace("sub_CT_", "").replace("CT_", "").split(".")[0]
            if num == "06":
                continue
            out = self.train_dir / f"{prefix}{num}.mha"
            self._log(f"DVF3D: Registering {m.name} -> fixed (CT_06)")
            generate_dvf(fixed, m, param_file, out)
            done += 1
        self._log(f"DVF3D ({mode}): Complete ({done} DVFs)")

    def _run_kv_preprocess(self):
        from modules.kv_preprocess.process import copy_test_data, process_kv_images
        if self.rt_list:
            copy_test_data(self.rt_list[0], self.base_dir, self.train_dir.parent)
        test_dir = self.train_dir.parent / "test"
        if test_dir.exists():
            process_kv_images(test_dir)
        ct06 = self.train_dir / "CT_06.mha"
        if ct06.exists():
            shutil.copy2(ct06, self.train_dir / "source.mha")
        self._log("KV_PREPROCESS: Complete")

    def cancel(self):
        self._cancelled = True
