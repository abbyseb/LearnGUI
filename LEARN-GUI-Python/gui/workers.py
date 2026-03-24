# workers.py - Python-Native Step Workers
from __future__ import annotations
from pathlib import Path
from typing import List, Optional
import os
import shutil
import traceback
from PyQt6.QtCore import QThread, pyqtSignal
from PyQt6.QtGui import QPixmap, QImage
import numpy as np
import pydicom
import time

# Import local modules
try:
    from modules.dicom2mha.convert import convert_dicom_to_mha
    from modules.downsampling.downsample import process_directory as downsample_dir
    from modules.drr_generation.generate import generate_drrs
    from modules.drr_compression.compress import process_directory as compress_dir
    from modules.dvf_generation.generate import generate_dvf
    from .run_preparation import build_train_pairs_for_run
except ImportError:
    pass

class BasePythonWorker(QThread):
    """Base class for all Python step workers."""
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
    """Worker for the COPY step."""
    preview_frame_ready = pyqtSignal(object, dict)
    
    def __init__(self, patient_id, rt_list, src_base, dst_base, parent=None):
        super().__init__(parent)
        self.patient_id = patient_id
        self.rt_list = rt_list
        self.src_base = Path(src_base)
        self.dst_base = Path(dst_base)

    def run(self):
        try:
            from gui.run_preparation import build_train_pairs_for_run
            pairs = build_train_pairs_for_run(self.rt_list, self.dst_base, self.src_base)
            
            self.log(f"Starting COPY for {self.patient_id}")
            self.log(f"Resolved {len(pairs)} source/destination pairs.")
            for i, (src, dst) in enumerate(pairs, 1):
                self.log(f"Pair {i}: SRC={src} -> DST={dst}")

            total_copied = 0
            
            for src, dst in pairs:
                if self._cancelled: break
                if not src.exists():
                    self.log(f"ERROR: Source directory not found: {src}", level="ERROR")
                    continue
                
                self.log(f"Processing source: {src.name}...")
                dst.mkdir(parents=True, exist_ok=True)
                
                for root, _, files in os.walk(src):
                    if self._cancelled: break
                    rel = Path(root).relative_to(src)
                    target = dst / rel
                    target.mkdir(parents=True, exist_ok=True)
                    
                    for f in files:
                        if self._cancelled: break
                        src_f = Path(root) / f
                        dst_f = target / f
                        if not dst_f.exists():
                            # DICOM Preview
                            if self._is_dicom(src_f):
                                self._emit_preview(src_f, total_copied)
                            shutil.copy2(src_f, dst_f)
                            total_copied += 1
            
            self.log(f"COPY complete: {total_copied} files.")
            self.finished_ok.emit("COPY finished")
        except Exception as e:
            self.failed.emit(str(e))

    def _is_dicom(self, path: Path) -> bool:
        try:
            with open(path, 'rb') as f:
                f.seek(128)
                return f.read(4) == b'DICM'
        except: return False

    def _emit_preview(self, path: Path, count: int):
        try:
            ds = pydicom.dcmread(str(path))
            pixels = ds.pixel_array.astype(np.float32)
            slope = float(getattr(ds, 'RescaleSlope', 1.0) or 1.0)
            intercept = float(getattr(ds, 'RescaleIntercept', 0.0) or 0.0)
            pixels = pixels * slope + intercept
            wl, ww = 400, 1500
            lo, hi = wl - ww/2, wl + ww/2
            u8 = np.clip((pixels - lo) / ww * 255.0, 0, 255).astype(np.uint8)
            h, w = u8.shape
            qimg = QImage(u8.data, w, h, w, QImage.Format.Format_Grayscale8)
            pix = QPixmap.fromImage(qimg.copy())
            self.preview_frame_ready.emit(pix, {'source': path.name, 'count': str(count+1)})
        except: pass

class Dicom2MhaWorker(BasePythonWorker):
    """Worker for DICOM to MHA conversion."""
    def __init__(self, dicom_dir, sample_path, parent=None):
        super().__init__(parent)
        self.dicom_dir = dicom_dir
        self.sample_path = sample_path

    def run(self):
        try:
            from modules.dicom2mha.convert import convert_dicom_to_mha
            self.log(f"Starting DICOM2MHA. Source: {self.dicom_dir}")
            self.log(f"Output will be near: {self.sample_path}")
            
            convert_dicom_to_mha(self.dicom_dir, self.sample_path)
            
            self.log("DICOM2MHA conversion complete.")
            self.finished_ok.emit("DICOM2MHA finished")
        except Exception as e:
            self.failed.emit(str(e))

class DownsampleWorker(BasePythonWorker):
    """Worker for Downsampling."""
    def __init__(self, train_dir, parent=None):
        super().__init__(parent)
        self.train_dir = train_dir

    def run(self):
        try:
            from modules.downsampling.downsample import process_directory as downsample_dir
            self.log(f"Starting DOWNSAMPLE for directory: {self.train_dir}")
            
            downsample_dir(str(self.train_dir))
            
            self.log("DOWNSAMPLE complete.")
            self.finished_ok.emit("DOWNSAMPLE finished")
        except Exception as e:
            self.failed.emit(str(e))

class DrrWorker(BasePythonWorker):
    """Worker for DRR generation."""
    def __init__(self, train_dir, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)

    def run(self):
        try:
            from modules.drr_generation.generate import generate_drrs
            self.log(f"Starting DRR generation for: {self.train_dir}")
            
            # Use RTKGeometry_360.xml from modules/drr_generation
            geom_file = Path(__file__).parent.parent / "modules" / "drr_generation" / "RTKGeometry_360.xml"
            self.log(f"Using geometry: {geom_file}")
            
            generate_drrs(str(self.train_dir), str(geom_file))
            
            self.log("DRR generation complete.")
            self.finished_ok.emit("DRR finished")
        except Exception as e:
            self.failed.emit(str(e))

class CompressWorker(BasePythonWorker):
    """Worker for DRR compression."""
    def __init__(self, train_dir, parent=None):
        super().__init__(parent)
        self.train_dir = train_dir

    def run(self):
        try:
            from modules.drr_compression.compress import process_directory as compress_dir
            self.log(f"Starting DRR compression (HNC -> PNG/TIF) in: {self.train_dir}")
            
            compress_dir(str(self.train_dir))
            
            self.log("DRR compression complete.")
            self.finished_ok.emit("COMPRESS finished")
        except Exception as e:
            self.failed.emit(str(e))

class DvfWorker(BasePythonWorker):
    """Worker for DVF generation."""
    def __init__(self, train_dir, do_low=True, do_full=False, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)
        self.do_low = do_low
        self.do_full = do_full

    def run(self):
        try:
            from modules.dvf_generation.generate import generate_dvf
            param_file = Path(__file__).parent.parent / "modules" / "dvf_generation" / "parameters.txt"
            self.log(f"Starting DVF generation. Params: {param_file}")
            
            if self.do_low:
                fixed = self.train_dir / "sub_CT_01.mha"
                moving = self.train_dir / "sub_CT_06.mha"
                self.log(f"Checking sub-DVFs: {fixed.name}, {moving.name}")
                if fixed.exists() and moving.exists():
                    self.log("Generating DVF_sub (01-06)...")
                    generate_dvf(str(fixed), str(moving), str(param_file), str(self.train_dir / "DVF_sub.mha"))
                else:
                    self.log(f"Skip sub-DVF: required files missing.")
            
            if self.do_full:
                fixed = self.train_dir / "CT_01.mha"
                moving = self.train_dir / "CT_06.mha"
                self.log(f"Checking full-DVFs: {fixed.name}, {moving.name}")
                if fixed.exists() and moving.exists():
                    self.log("Generating DVF_full (01-06)...")
                    generate_dvf(str(fixed), str(moving), str(param_file), str(self.train_dir / "DVF_full_01_06.mha"))
                else:
                    self.log(f"Skip full-DVF: required files missing.")
            
            self.log("DVF generation finished.")
            self.finished_ok.emit("DVF finished")
        except Exception as e:
            self.failed.emit(str(e))

class Dvf2dWorker(BasePythonWorker):
    """Worker for 2D DVF generation."""
    def __init__(self, train_dir, parent=None):
        super().__init__(parent)
        self.train_dir = Path(train_dir)

    def run(self):
        try:
            from modules.dvf_generation.generate import generate_dvf
            self.log("Starting 2D DVF generation (projections)...")
            # In Python native, we might use a different module or placeholder
            # For now, simulate work as per the original Job logic
            time.sleep(1.0) 
            self.finished_ok.emit("2D DVF finished")
        except Exception as e:
            self.failed.emit(str(e))

class KvPreprocessWorker(BasePythonWorker):
    """Placeholder for KV Preprocessing."""
    def run(self):
        self.log("Starting KV_PREPROCESS...")
        time.sleep(0.5)
        self.finished_ok.emit("KV_PREPROCESS finished")
