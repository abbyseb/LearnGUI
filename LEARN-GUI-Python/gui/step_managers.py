# step_managers.py
"""
Self-contained step managers for Voxelmap pipeline.
Each step knows:
- What files to watch for progress
- What viewer to activate
- When it's complete
- How to calculate percentage

Works identically for normal runs and reruns.
"""
from __future__ import annotations
from pathlib import Path
from typing import Optional, List
from PyQt6.QtCore import QObject, pyqtSignal, QTimer
import time
import re

def get_projections_per_ct() -> int:
    """Read projection count from modules/drr_generation/RTKGeometry_360.xml or default 120."""
    try:
        root = Path(__file__).resolve().parents[1]
        geom_path = root / "modules" / "drr_generation" / "RTKGeometry_360.xml"
        if geom_path.exists():
            content = geom_path.read_text(encoding='utf-8')
            # Each projection is marked by <Projection> tag
            count = len(re.findall(r"<Projection>", content))
            if count > 0:
                return count
    except Exception as e:
        print(f"Warning: Could not detect projection count: {e}")
    return 120 # Fallback


class StepManager(QObject):
    """
    Base class for step-specific progress management.
    
    Each step manager:
    - Monitors train/ for expected outputs
    - Calculates progress percentage
    - Activates appropriate viewer
    - Signals completion
    """
    
    progress_updated = pyqtSignal(int, str)  # percentage, status_text
    step_complete = pyqtSignal()
    activate_viewer = pyqtSignal(str)  # viewer_name: 'ct', 'drr', 'none'
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.train_dir: Optional[Path] = None
        self.is_active = False
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._poll)
        self._last_progress = -1
        self._completion_checks = 0
        
    def start(self, train_dir: Path):
        """Activate this step's monitoring."""
        self.train_dir = Path(train_dir)
        self.is_active = True
        self._last_progress = -1
        self._completion_checks = 0
        
        # Activate viewer first
        viewer = self.get_viewer_type()
        if viewer:
            self.activate_viewer.emit(viewer)
        
        # Start polling
        interval = self.get_poll_interval_ms()
        self._timer.start(interval)
        
        # Immediate first poll
        self._poll()
    
    def stop(self):
        """Deactivate monitoring."""
        self.is_active = False
        self._timer.stop()
    
    def _poll(self):
        """Poll train directory and emit progress."""
        if not self.is_active or not self.train_dir:
            return
        
        try:
            created, total, pct = self.calculate_progress()
            status = self.format_status(created, total)
            
            # Emit if changed
            if pct != self._last_progress:
                self.progress_updated.emit(pct, status)
                self._last_progress = pct
            
            # Check completion (require 3 consecutive polls at 100%)
            if self.is_complete(created, total):
                self._completion_checks += 1
                if self._completion_checks >= 3:
                    self.stop()
                    self.step_complete.emit()
            else:
                self._completion_checks = 0
                
        except Exception as e:
            print(f"Poll error in {self.__class__.__name__}: {e}")
    
    # Abstract methods - override in subclasses
    def get_viewer_type(self) -> Optional[str]:
        """Return 'ct', 'drr', or None."""
        return None
    
    def get_poll_interval_ms(self) -> int:
        """Poll frequency in milliseconds."""
        return 250
    
    def calculate_progress(self) -> tuple[int, int, int]:
        """Return (created_count, total_count, percentage)."""
        raise NotImplementedError
    
    def format_status(self, created: int, total: int) -> str:
        """Format status text for progress bar."""
        raise NotImplementedError
    
    def is_complete(self, created: int, total: int) -> bool:
        """Check if step is done."""
        return total > 0 and created >= total


class WorkerBackedStepManager(StepManager):
    """
    Runs the real work in a QThread worker (see workers.py). Polling only refreshes
    the progress bar; step completion is signaled when the worker emits finished_ok.
    """

    _step_key: str = "STEP"
    _status_when_done_text: str = "Complete."

    def __init__(self, dataset_type: str = "clinical", parent=None):
        super().__init__(parent)
        self.dataset_type = dataset_type
        self._worker = None

    def _poll(self):
        if not self.is_active or not self.train_dir:
            return
        try:
            created, total, pct = self.calculate_progress()
            status = self.format_status(created, total)
            if pct != self._last_progress:
                self.progress_updated.emit(pct, status)
                self._last_progress = pct
        except Exception as e:
            print(f"Poll error in {self.__class__.__name__}: {e}")

    def start(self, train_dir: Path):
        super().start(train_dir)
        self._start_worker()

    def _create_worker(self):
        raise NotImplementedError

    def _start_worker(self):
        w = self._create_worker()
        if w is None:
            return
        self._worker = w
        w.line_received.connect(self._on_worker_log)
        w.finished_ok.connect(self._on_worker_finished)
        w.failed.connect(self._on_worker_failed)
        w.start()

    def _on_worker_log(self, msg: str) -> None:
        c = self.parent()
        if c and hasattr(c, "log"):
            c.log(msg)

    def _on_worker_finished(self, _name: str) -> None:
        if not self.is_active:
            return
        self._detach_worker()
        self.is_active = False
        self._timer.stop()
        self.progress_updated.emit(100, self._status_when_done_text)
        self.step_complete.emit()

    def _on_worker_failed(self, err: str) -> None:
        self._detach_worker()
        c = self.parent()
        if c and hasattr(c, "_finish_err"):
            c._finish_err(f"{self._step_key}: {err}")
        else:
            print(f"{self._step_key} failed: {err}")

    def _detach_worker(self) -> None:
        w = self._worker
        if not w:
            return
        for sig, slot in (
            (w.line_received, self._on_worker_log),
            (w.finished_ok, self._on_worker_finished),
            (w.failed, self._on_worker_failed),
        ):
            try:
                sig.disconnect(slot)
            except TypeError:
                pass
        self._worker = None
        w.deleteLater()

    def stop(self):
        w = getattr(self, "_worker", None)
        if w is not None:
            if w.isRunning():
                w.cancel()
                w.wait(8000)
            self._detach_worker()
        super().stop()


class CopyStepManager(StepManager):
    """Runs parallel file copy (CopyWorker) and DICOM preview while COPY is active."""
    
    # files_done, files_total, dicom_arrived, dicom_expected (from prescription count)
    progress_detail = pyqtSignal(int, int, int, int)
    
    def __init__(
        self,
        total_dicoms: int,
        patient_id: str = "",
        rt_list: Optional[List] = None,
        src_base: str = "",
        dst_base: Path | str = "",
        dataset_type: str = "clinical",
        parent=None,
    ):
        super().__init__(parent)
        self.total_dicoms = int(total_dicoms)
        self.dataset_type = dataset_type
        self.patient_id = patient_id or ""
        self.rt_list = list(rt_list or [])
        self.src_base = str(src_base or "")
        self.dst_base = Path(dst_base) if dst_base else Path()
        self.baseline_count = 0
        self._files_done = 0
        self._files_total = 0
        self._last_copy_ft_reported = -2  # sentinel: first emit after start
        self._preview_engine = None
        self._copy_worker = None
    
    def start(self, train_dir: Path):
        c = self.parent()
        if not self.rt_list or not self.dst_base or not self.src_base:
            msg = "COPY: missing rt_list, dst_base, or src_base — cannot copy."
            if c and hasattr(c, "log"):
                c.log(msg, level="ERROR")
            if c and hasattr(c, "_finish_err"):
                c._finish_err(msg)
            return
        self.baseline_count = self._count_dicoms(train_dir)
        self._files_done = 0
        self._files_total = 0
        self._last_copy_ft_reported = -2
        super().start(train_dir)
        self._start_copy_worker()
        # For clinical: start DICOM carousel preview during copy
        # For SPARE: MHA triptych is loaded AFTER copy completes (see _on_copy_worker_finished)
        if self.dataset_type != "spare":
            self._start_dicom_preview()
    
    def _disconnect_copy_worker(self) -> None:
        w = self._copy_worker
        if not w:
            return
        for sig, slot in (
            (w.line_received, self._on_copy_log_line),
            (w.file_progress, self._on_copy_file_progress),
            (w.finished_ok, self._on_copy_worker_finished),
            (w.failed, self._on_copy_worker_failed),
        ):
            try:
                sig.disconnect(slot)
            except TypeError:
                pass
        self._copy_worker = None
        w.deleteLater()

    def _stop_copy_worker_if_running(self) -> None:
        w = self._copy_worker
        if not w:
            return
        if w.isRunning():
            w.cancel()
            w.wait(8000)
        self._disconnect_copy_worker()

    def _cleanup_dicom_preview_ui(self) -> None:
        controller = self.parent()
        # For SPARE, the triptych was loaded directly — don't clear it
        if self.dataset_type == "spare":
            # Just stop the preview engine if any
            if self._preview_engine:
                try:
                    self._preview_engine.stop()
                except Exception:
                    pass
                self._preview_engine = None
            return
        
        if hasattr(controller, "win"):
            viewer = controller.win.tab2d.ct_quad
            if hasattr(viewer, "info_panel"):
                try:
                    viewer.info_panel.dicom_preview_series_uid_changed.disconnect(
                        self._on_copy_preview_series_uid
                    )
                except TypeError:
                    pass
                viewer.info_panel.clear_dicom_preview_series()
                from .ui.info_panels import CTInfoWidget

                if isinstance(viewer.info_panel, CTInfoWidget):
                    viewer.info_panel.set_preview_mode(False)
        if self._preview_engine:
            try:
                self._preview_engine.series_catalog_changed.disconnect(
                    self._on_copy_series_catalog
                )
            except TypeError:
                pass
            try:
                self._preview_engine.stop()
            except Exception:
                pass
            self._preview_engine = None

    def stop(self):
        self._stop_copy_worker_if_running()
        super().stop()
        self._cleanup_dicom_preview_ui()

    def _on_copy_log_line(self, msg: str) -> None:
        c = self.parent()
        if c and hasattr(c, "log"):
            c.log(msg)

    def _on_copy_file_progress(self, done: int, total: int) -> None:
        self._files_done = int(done)
        self._files_total = int(total)
        self._emit_copy_progress_ui()

    def _emit_copy_progress_ui(self) -> None:
        """Progress bar follows all files; detail line shows files + DICOMs on disk."""
        if not self.train_dir:
            return
        d_arrived = max(0, self._count_dicoms(self.train_dir, self.dataset_type) - self.baseline_count)
        d_exp = max(0, self.total_dicoms)
        fd, ft = self._files_done, self._files_total
        self.progress_detail.emit(fd, ft, d_arrived, d_exp)
        if ft > 0:
            pct = int(min(99, (fd / ft) * 100))
            if fd >= ft:
                status = "Copying training files (all files written; finishing)…"
            else:
                status = "Copying training files…"
        else:
            pct = 0
            status = "Preparing file copy list…"
        ft_changed = self._last_copy_ft_reported != ft
        if pct != self._last_progress or ft_changed:
            self.progress_updated.emit(pct, status)
            self._last_progress = pct
            self._last_copy_ft_reported = ft

    def _on_copy_worker_finished(self, _name: str) -> None:
        if not self.is_active:
            return
        self._stop_copy_worker_if_running()
        self.is_active = False
        self._timer.stop()
        self._cleanup_dicom_preview_ui()
        
        # For SPARE: load MHA triptych NOW that files are in train_dir
        if self.dataset_type == "spare":
            self._start_mha_volume_preview()
        
        self.progress_updated.emit(100, "Copy complete.")
        self.step_complete.emit()

    def _on_copy_worker_failed(self, err: str) -> None:
        self._stop_copy_worker_if_running()
        c = self.parent()
        if c and hasattr(c, "_finish_err"):
            c._finish_err(f"COPY: {err}")
        else:
            print(f"COPY failed: {err}")
    
    def get_viewer_type(self) -> Optional[str]:
        # Return None to prevent the main controller from switching the viewer.
        # The Copy step manages its own DICOM preview on the initial panel.
        return None
    
    def calculate_progress(self) -> tuple[int, int, int]:
        """Satisfy StepManager API; bar is driven by _emit_copy_progress_ui (file counts)."""
        fd, ft = self._files_done, self._files_total
        pct = int(min(99, (fd / ft) * 100)) if ft > 0 else 0
        return fd, ft, pct

    def format_status(self, created: int, total: int) -> str:
        return "Copying training files…"

    def _poll(self):
        """Refresh DICOM arrival counts while the worker streams file_progress."""
        if not self.is_active or not self.train_dir:
            return
        try:
            self._emit_copy_progress_ui()
        except Exception as e:
            print(f"Poll error in {self.__class__.__name__}: {e}")

    def _start_copy_worker(self) -> None:
        """Parallel copy into train/ (ThreadPoolExecutor in CopyWorker)."""
        from .workers import CopyWorker

        self._copy_worker = CopyWorker(
            patient_id=self.patient_id,
            rt_list=self.rt_list,
            src_base=self.src_base,
            dst_base=self.dst_base,
            parent=self,
        )
        self._copy_worker.line_received.connect(self._on_copy_log_line)
        self._copy_worker.file_progress.connect(self._on_copy_file_progress)
        self._copy_worker.finished_ok.connect(self._on_copy_worker_finished)
        self._copy_worker.failed.connect(self._on_copy_worker_failed)
        self._copy_worker.start()
    
    def _start_mha_volume_preview(self):
        """For SPARE: load first MHA directly into CTQuadPanel triptych (3-panel view)."""
        c = self.parent()
        def _log(msg):
            if c and hasattr(c, 'log'):
                c.log(msg)
            print(msg)
        
        try:
            import SimpleITK as sitk
            import numpy as np
            
            _log(f"DEBUG: _start_mha_volume_preview called. train_dir={self.train_dir}")
            
            controller = self.parent()
            if not hasattr(controller, 'win'):
                _log("DEBUG: MHA preview: no controller.win — aborting")
                return
            viewer = controller.win.tab2d.ct_quad
            
            # Find MHA files in train_dir
            if not self.train_dir:
                _log("DEBUG: MHA preview: train_dir is None — aborting")
                return
            if not self.train_dir.exists():
                _log(f"DEBUG: MHA preview: train_dir does not exist: {self.train_dir}")
                return
            
            mha_files = sorted(self.train_dir.glob('*.mha'))
            _log(f"DEBUG: MHA preview: found {len(mha_files)} .mha files in {self.train_dir}")
            if not mha_files:
                return
            
            # Pick the first volume (typically a CT or GTVol)
            mha_path = mha_files[0]
            _log(f"INFO: Loading MHA preview: {mha_path.name} ({mha_path.stat().st_size / 1e6:.1f} MB)")
            
            img = sitk.ReadImage(str(mha_path))
            arr = sitk.GetArrayFromImage(img).astype(np.float32)
            spacing = img.GetSpacing()  # (sx, sy, sz) in ITK order
            spacing_zyx = (float(spacing[2]), float(spacing[1]), float(spacing[0]))
            _log(f"DEBUG: MHA read OK. shape={arr.shape}, spacing_zyx={spacing_zyx}")
            
            viewer.set_mha_volume(
                arr, spacing_zyx,
                origin=img.GetOrigin(),
                direction=img.GetDirection(),
                sitk_image=img
            )
            _log("DEBUG: set_mha_volume() completed — triptych should be visible")
            
            # Update info to show which volume is displayed
            z, y, x = arr.shape
            viewer.set_info(
                source=mha_path.name,
                dims=f"{z} × {y} × {x}",
                vox=f"{spacing_zyx[0]:.3f}, {spacing_zyx[1]:.3f}, {spacing_zyx[2]:.3f}",
                count=str(len(mha_files)),
            )
            
            # If there are multiple MHA files, populate the phase selector
            if len(mha_files) > 1 and hasattr(viewer, 'set_phase_options'):
                names = [p.stem for p in mha_files]
                viewer.set_phase_options(names, 0)
            
        except Exception as e:
            _log(f"ERROR: MHA volume preview failed: {e}")
            import traceback
            traceback.print_exc()

    def _start_dicom_preview(self):
        """Start efficient DICOM preview engine."""
        try:
            from .dicom_preview import DICOMPreviewEngine
            from .ui.info_panels import CTInfoWidget 
            
            # Get viewer reference
            controller = self.parent()
            if not hasattr(controller, 'win'):
                return
            
            viewer = controller.win.tab2d.ct_quad

            if not (
                hasattr(viewer, "info_panel")
                and isinstance(viewer.info_panel, CTInfoWidget)
            ):
                return

            viewer.info_panel.set_preview_mode(True)

            self._preview_engine = DICOMPreviewEngine(
                self.train_dir,
                display_fps=5,  # Smooth 5 FPS
                discovery_interval_ms=150,  # Check for new files frequently
                parent=self
            )

            viewer.info_panel.dicom_preview_series_uid_changed.connect(
                self._on_copy_preview_series_uid
            )
            self._preview_engine.series_catalog_changed.connect(self._on_copy_series_catalog)
            
            # Connect to viewer updates (only axial visible in single/preview mode)
            def update_preview(pix_ax, pix_co, pix_sa, info):
                try:
                    if pix_ax and not pix_ax.isNull():
                        viewer.set_axial_pixmap(pix_ax)
                    if info:
                        viewer.set_info_dict(info)
                except Exception as e:
                    print(f"Preview update error: {e}")
            
            self._preview_engine.frame_ready.connect(update_preview)
            self._preview_engine.start()
            
        except Exception as e:
            print(f"DICOM preview engine failed: {e}")
            import traceback
            traceback.print_exc()

    def _on_copy_preview_series_uid(self, uid: str) -> None:
        if self._preview_engine:
            self._preview_engine.set_active_series_uid(uid)

    def _on_copy_series_catalog(self, opts: list) -> None:
        controller = self.parent()
        if not hasattr(controller, "win"):
            return
        viewer = controller.win.tab2d.ct_quad
        if hasattr(viewer, "info_panel"):
            viewer.info_panel.update_dicom_preview_series(list(opts))
    
    @staticmethod
    def _count_dicoms(path: Path, dataset_type: str = "clinical") -> int:
        """Count source files recursively (DICOMs or MHAs)."""
        from .run_preparation import count_dicoms_recursive
        return count_dicoms_recursive(path, dataset_type)


class DICOM2MHAStepManager(WorkerBackedStepManager):
    """DICOM → MHA via Dicom2MhaWorker; poll reflects CT_*.mha growth."""

    _step_key = "DICOM2MHA"
    _status_when_done_text = "DICOM → MHA complete."

    progress_detail = pyqtSignal(int, int, int, int)  # ct_done, ct_total, struct_done, struct_total

    def __init__(
        self,
        dataset_type: str = "clinical",
        transpose_axes: Optional[tuple[int, int, int]] = None,
        rotate_k: int = 0,
        parent=None,
    ):
        super().__init__(dataset_type=dataset_type, parent=parent)
        self.transpose_axes = transpose_axes
        self.rotate_k = rotate_k

    def _create_worker(self):
        from .workers import Dicom2MhaWorker
        return Dicom2MhaWorker(
            self.train_dir,
            dataset_type=self.dataset_type,
            transpose_axes=self.transpose_axes,
            rotate_k=self.rotate_k,
            parent=self,
        )

    def get_viewer_type(self) -> Optional[str]:
        return 'ct'
    
    def get_poll_interval_ms(self) -> int:
        return 500  # MHA files are large, slower polling is fine
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, -1, 0
        
        # Count created MHAs
        ct_created = len(list(self.train_dir.glob("CT_*.mha")))
        struct_names = ["Body.mha", "GTV_Inh.mha", "GTV_Exh.mha", "Lungs_Inh.mha", "Lungs_Exh.mha"]
        struct_found = [n for n in struct_names if (self.train_dir / n).exists()]
        struct_created = len(struct_found)
        
        # Try to get expected count from SeriesInfo.txt (written at start of conversion)
        ct_expected = self._read_expected_ct_count()
        if ct_expected is None:
            # Indeterminate state
            self.progress_detail.emit(ct_created, -1, struct_created, 5)
            return ct_created, -1, 0
        
        # Emit detail for right label
        self.progress_detail.emit(ct_created, ct_expected, struct_created, 5)
        
        # Weighted progress
        # Many datasets (like VALKIM) don't have all structures. 
        # We give 90% weight to CTs and 10% to "any" structure found.
        ct_pct = (ct_created / ct_expected) * 90 if ct_expected > 0 else 0
        struct_pct = 10 if struct_created > 0 else 0
        
        pct = int(ct_pct + struct_pct)
        
        # If we have all CTs, we are at least at 90%
        if ct_created >= ct_expected and pct < 90:
            pct = 90

        total = ct_expected + 5
        created = ct_created + struct_created
        
        return created, total, pct
    
    def is_complete(self, created: int, total: int) -> bool:
        """Complete when all CTs are done (structures optional; VALKIM has none)."""
        ct_expected = self._read_expected_ct_count()
        if ct_expected is None or ct_expected <= 0:
            return False
        ct_created = len(list(self.train_dir.glob("CT_*.mha"))) if self.train_dir and self.train_dir.exists() else 0
        return ct_created >= ct_expected
    
    def format_status(self, created: int, total: int) -> str:
        if self.dataset_type == "spare":
            return "Normalizing Nomenclature (GTVol → CT)..."
        if total == -1:
            return "Converting DICOM → MHA... (scanning)"
        return "Converting DICOM → MHA..."
    
    def _read_expected_ct_count(self) -> Optional[int]:
        """Parse SeriesInfo.txt for expected CT count."""
        series_info = self.train_dir / "SeriesInfo.txt"
        if not series_info.exists():
            return None
        try:
            import re
            count = 0
            with series_info.open("r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    if re.search(r"\bCT_\d{2}\s*:", line):
                        count += 1
            return count if count > 0 else None
        except Exception:
            return None


class DownsampleStepManager(WorkerBackedStepManager):
    """Downsample via DownsampleWorker."""

    _step_key = "DOWNSAMPLE"
    _status_when_done_text = "Downsample complete."

    progress_detail = pyqtSignal(int, int)  # created, total

    def __init__(self, dataset_type: str = "clinical", parent=None):
        super().__init__(dataset_type=dataset_type, parent=parent)

    def _create_worker(self):
        from .workers import DownsampleWorker
        return DownsampleWorker(self.train_dir, dataset_type=self.dataset_type, parent=self)

    def get_viewer_type(self) -> Optional[str]:
        return None  # Keep previous viewer
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        ct_count = len(list(self.train_dir.glob("CT_*.mha")))
        sub_ct_count = len(list(self.train_dir.glob("sub_CT_*.mha")))
        
        self.progress_detail.emit(sub_ct_count, ct_count)
        
        pct = int(min(100, (sub_ct_count / ct_count * 100))) if ct_count > 0 else 0
        
        return sub_ct_count, ct_count, pct
    
    def is_complete(self, created: int, total: int) -> bool:
        """Complete when all CTs downsampled (VALKIM has no Lungs, so sub_CT >= CT is sufficient)."""
        if total <= 0:
            return False
        if not self.train_dir or not self.train_dir.exists():
            return False
        ct_count = len(list(self.train_dir.glob("CT_*.mha")))
        sub_ct_count = len(list(self.train_dir.glob("sub_CT_*.mha")))
        return sub_ct_count >= ct_count and ct_count > 0
    
    def format_status(self, created: int, total: int) -> str:
        return "Downsampling volumes..."


class DRRStepManager(WorkerBackedStepManager):
    """DRR generation via DrrWorker; poll reflects projection files on disk."""

    _step_key = "DRR"
    _status_when_done_text = "DRR generation complete."
    _proj_cache: Optional[int] = None

    progress_detail = pyqtSignal(str)  # detail text

    def __init__(
        self,
        geom_path: Path | str | None = None,
        drr_opts: dict | None = None,
        dataset_type: str = "clinical",
        parent=None,
    ):
        super().__init__(dataset_type=dataset_type, parent=parent)
        self._geom_path = geom_path
        self._drr_opts = dict(drr_opts or {})
        self.dataset_type = dataset_type

    def _default_geom_path(self) -> Path:
        return Path(__file__).resolve().parents[1] / "modules" / "drr_generation" / "RTKGeometry_360.xml"

    def _create_worker(self):
        from .workers import DrrWorker

        gp = self._geom_path
        if not gp:
            gp = self._default_geom_path()
        return DrrWorker(
            self.train_dir, Path(gp), drr_opts=self._drr_opts, dataset_type=self.dataset_type, parent=self
        )

    @property
    def projections_per_ct(self) -> int:
        if DRRStepManager._proj_cache is None:
            DRRStepManager._proj_cache = get_projections_per_ct()
        return DRRStepManager._proj_cache

    def get_viewer_type(self) -> Optional[str]:
        return 'drr'
    
    def get_poll_interval_ms(self) -> int:
        return 300
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        # Group files by CT
        ct_groups = self._group_by_ct()
        total_cts = self._get_ct_count()
        
        if not ct_groups or total_cts == 0:
            self.progress_detail.emit(f"Waiting for CTs... (0/{total_cts})")
            return 0, 0, 0
        
        # Count total created
        total_created = sum(len(files) for files in ct_groups.values())
        total_expected = total_cts * self.projections_per_ct
        
        # Find current CT being processed
        completed_cts = sum(1 for files in ct_groups.values() 
                          if len(files) >= self.projections_per_ct)
        
        current_ct = None
        current_count = 0
        for ct_num in sorted(ct_groups.keys()):
            files = ct_groups[ct_num]
            if len(files) < self.projections_per_ct:
                current_ct = ct_num
                current_count = len(files)
                break
        
        # Build detail string
        if current_ct:
            detail = f"CT_{current_ct:02d}/{total_cts:02d} • {current_count}/{self.projections_per_ct}"
        elif completed_cts == total_cts:
            detail = f"Complete • {total_cts} CTs"
        else:
            detail = f"Waiting... ({completed_cts}/{total_cts} CTs done)"
        
        self.progress_detail.emit(detail)
        
        pct = int(min(100, (total_created / total_expected * 100))) if total_expected > 0 else 0
        
        return total_created, total_expected, pct
    
    def format_status(self, created: int, total: int) -> str:
        return "Generating DRRs..."
    
    def _group_by_ct(self) -> dict:
        """Group DRR projection files by CT number (tif during generation, bin after compress)."""
        import re
        groups = {}
        
        # Support .tif/.tiff (Python DRR output), .hnc (MATLAB), .bin (post-compress)
        patterns = ["*_Proj_*.tif", "*_Proj_*.tiff", "*_Proj_*.png", "*_Proj_*.hnc", "*_Proj_*.bin"]
        
        for pattern in patterns:
            for file in self.train_dir.glob(pattern):
                match = re.search(r"(\d+)_Proj_(\d+)\.(tif|tiff|png|hnc|bin)", file.name, re.IGNORECASE)
                if match:
                    ct_num = int(match.group(1))
                    if ct_num not in groups:
                        groups[ct_num] = set()
                    groups[ct_num].add(file.name)
        
        return groups
    
    def _get_ct_count(self) -> int:
        """Match generate.py: full-res CTs first, else downsampled."""
        n = len(list(self.train_dir.glob("CT_*.mha")))
        if n == 0:
            n = len(list(self.train_dir.glob("sub_CT_*.mha")))
        return n


class CompressStepManager(WorkerBackedStepManager):
    """DRR compression via CompressWorker."""

    _step_key = "COMPRESS"
    _status_when_done_text = "DRR compression complete."

    progress_detail = pyqtSignal(int, int)  # created, total

    def __init__(self, dataset_type: str = "clinical", parent=None):
        super().__init__(dataset_type=dataset_type, parent=parent)

    def _create_worker(self):
        from .workers import CompressWorker
        return CompressWorker(self.train_dir, dataset_type=self.dataset_type, parent=self)

    @property
    def projections_per_ct(self) -> int:
        return get_projections_per_ct()

    def get_viewer_type(self) -> Optional[str]:
        return None  # Keep DRR viewer
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        n_ct = len(list(self.train_dir.glob("CT_*.mha")))
        total_cts = n_ct if n_ct > 0 else len(list(self.train_dir.glob("sub_CT_*.mha")))
        ppc = self.projections_per_ct
        expected = total_cts * ppc if total_cts > 0 else ppc * 10 
        
        # Created: .bin files (the result of compression)
        created = len(list(self.train_dir.glob("*_Proj_*.bin")))
        
        # Emit detail
        self.progress_detail.emit(created, expected)
        
        pct = int(min(100, (created / expected * 100))) if expected > 0 else 0
        
        return created, expected, pct
    
    def format_status(self, created: int, total: int) -> str:
        return "Compressing DRRs..."


class DVF2DStepManager(WorkerBackedStepManager):
    """2D DVF placeholder worker (instant complete until external tool is wired)."""

    _step_key = "DVF2D"
    _status_when_done_text = "2D DVF step finished."

    def __init__(self, dataset_type: str = "clinical", parent=None):
        super().__init__(dataset_type=dataset_type, parent=parent)

    def _create_worker(self):
        from .workers import Dvf2DWorker
        return Dvf2DWorker(self.train_dir, parent=self)

    def get_viewer_type(self) -> Optional[str]:
        return None  # Keep DRR viewer
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        # Look for *2DDVF* files
        created = len(list(self.train_dir.glob("*2DDVF*")))
        
        # Expected: projection count from any pipeline stage (PNG/TIF before compress, bin/hnc after)
        projs = sum(
            len(list(self.train_dir.glob(p)))
            for p in (
                "*_Proj_*.png",
                "*_Proj_*.tif",
                "*_Proj_*.tiff",
                "*_Proj_*.hnc",
                "*_Proj_*.bin",
            )
        )
        
        if projs > 0 and created > 0:
            pct = min(100, int((created / projs) * 100))
        else:
            pct = 0
        
        return created, projs, pct
    
    def format_status(self, created: int, total: int) -> str:
        return f"Generating 2D DVFs..."


class DVF3DLowStepManager(WorkerBackedStepManager):
    """3D DVF (downsampled) via Dvf3DWorker."""

    _step_key = "DVF3D_LOW"
    _status_when_done_text = "3D DVF (downsampled) complete."

    progress_detail = pyqtSignal(int, int)  # created, total

    def __init__(self, dataset_type: str = "clinical", parent=None):
        super().__init__(dataset_type=dataset_type, parent=parent)

    def _create_worker(self):
        from .workers import Dvf3DWorker
        return Dvf3DWorker(self.train_dir, low_res=True, dataset_type=self.dataset_type, parent=self)

    def get_viewer_type(self) -> Optional[str]:
        return 'dvf'

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0

        # Expected: sub_CT count minus 1 (fixed = CT_06, moving = all others)
        sub_cts = list(self.train_dir.glob("sub_CT_*.mha"))
        total = max(0, len(sub_cts) - 1)  # exclude fixed phase 06
        if total == 0 and sub_cts:
            total = len(sub_cts) - 1

        # Worker outputs DVF_sub_01.mha, DVF_sub_02.mha, ... (skips 06)
        created = len(list(self.train_dir.glob("DVF_sub_*.mha")))
        
        self.progress_detail.emit(created, total)
        
        pct = int(min(100, (created / total * 100))) if total > 0 else 0
        return created, total, pct

    def is_complete(self, created: int, total: int) -> bool:
        if total <= 0:
            return False
        return created >= total

    def format_status(self, created: int, total: int) -> str:
        return "Generating 3D DVFs (downsampled)..."

class DVF3DFullStepManager(WorkerBackedStepManager):
    """3D DVF (full-res) via Dvf3DWorker."""

    _step_key = "DVF3D_FULL"
    _status_when_done_text = "3D DVF (full-res) complete."

    progress_detail = pyqtSignal(int, int)  # created, total

    def __init__(self, dataset_type: str = "clinical", parent=None):
        super().__init__(dataset_type=dataset_type, parent=parent)

    def _create_worker(self):
        from .workers import Dvf3DWorker
        return Dvf3DWorker(self.train_dir, low_res=False, dataset_type=self.dataset_type, parent=self)

    def get_viewer_type(self) -> Optional[str]:
        return 'dvf'

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0

        # Expected: CT count minus 1 (fixed = CT_06, moving = all others)
        cts = list(self.train_dir.glob("CT_*.mha"))
        total = max(0, len(cts) - 1)
        if total == 0 and cts:
            total = len(cts) - 1

        # Worker outputs DVF_01.mha, DVF_02.mha, ... (skips 06) - exclude DVF_sub_*
        all_full = [f for f in self.train_dir.glob("DVF_*.mha") if "sub_" not in f.name]
        created = len(all_full)
        
        self.progress_detail.emit(created, total)
        
        pct = int(min(100, (created / total * 100))) if total > 0 else 0
        return created, total, pct

    def is_complete(self, created: int, total: int) -> bool:
        if total <= 0:
            return False
        return created >= total

    def format_status(self, created: int, total: int) -> str:
        return "Generating 3D DVFs (full res)..."


class KVPreprocessStepManager(WorkerBackedStepManager):
    """kV image preprocessing (extract test data, reference source.mha)."""

    _step_key = "KV_PREPROCESS"
    _status_when_done_text = "kV Preprocessing complete."

    def __init__(self, rt_ct_pres_list: list, base_dir: str, dataset_type: str = "clinical", parent=None):
        super().__init__(dataset_type=dataset_type, parent=parent)
        self.rt_ct_pres_list = rt_ct_pres_list
        self.base_dir = base_dir
        self.total_fractions = 0

    def _create_worker(self):
        from .workers import KvPreprocessWorker

        pr = self.train_dir.parent if self.train_dir else Path()
        return KvPreprocessWorker(self.rt_ct_pres_list, self.base_dir, pr, parent=self)

    def get_viewer_type(self) -> Optional[str]:
        return "preparing"

    def start(self, train_dir: Path):
        self.total_fractions = self._find_total_fractions()
        super().start(train_dir)
        
    def _find_total_fractions(self) -> int:
        """Find expected fraction folders from source."""
        from .run_preparation import train_src_from_rt
        import re
        count = 0
        if not self.rt_ct_pres_list:
            return 1 # Avoid divide by zero
            
        rt = self.rt_ct_pres_list[0]
        # Get /VALKIM/RNSH/PlanningFiles/Patient01/
        planning_files_dir = train_src_from_rt(rt, self.base_dir).parent.parent
        treatment_files_dir = planning_files_dir.parent / "Treatment files"
        
        if not treatment_files_dir.exists():
            return 1
            
        try:
            for item in treatment_files_dir.iterdir():
                if item.is_dir() and re.match(r"^Fx\d+$", item.name, re.IGNORECASE):
                    count += 1
        except Exception:
            pass
        return count if count > 0 else 1

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir:
            return 0, 1, 0
            
        test_dir = self.train_dir.parent / "test"
        if not test_dir.exists():
            return 0, self.total_fractions, 0
        
        # Monitor .bin files as indicator of progress
        created = len(list(test_dir.rglob("*.bin")))
        
        # Count processed folders
        processed_folders = set()
        for bin_file in test_dir.rglob("*.bin"):
            try:
                processed_folders.add(bin_file.parent.parent.name)
            except Exception:
                pass
        
        processed_count = len(processed_folders)
        self.progress_detail.emit(processed_count, self.total_fractions)
        
        pct = int(min(100, (processed_count / self.total_fractions * 100)))
        return processed_count, self.total_fractions, pct

    def format_status(self, created: int, total: int) -> str:
        return f"Copying/Processing kV files..."


# Factory function
def create_step_manager(step_name: str, **kwargs) -> StepManager:
    """Create appropriate manager for step."""
    manager_class = {
        'COPY': CopyStepManager,
        'DICOM2MHA': DICOM2MHAStepManager,
        'DOWNSAMPLE': DownsampleStepManager,
        'DRR': DRRStepManager,
        'COMPRESS': CompressStepManager,
        'DVF2D': DVF2DStepManager,
        'DVF3D_LOW': DVF3DLowStepManager,
        'DVF3D_FULL': DVF3DFullStepManager,
        'KV_PREPROCESS': KVPreprocessStepManager,
    }.get(step_name.upper())
    
    if not manager_class:
        raise ValueError(f"Unknown step: {step_name}")
    
    # Pass only the arguments the constructor expects
    import inspect
    sig = inspect.signature(manager_class.__init__)
    # 1. Pop 'parent' from kwargs. It's passed explicitly.
    parent_arg = kwargs.pop('parent', None)
    
    # 2. Filter the *remaining* kwargs against the signature
    valid_kwargs = {k: v for k, v in kwargs.items() if k in sig.parameters}
    
    # 3. Call with the separated parent and valid_kwargs
    return manager_class(parent=parent_arg, **valid_kwargs)