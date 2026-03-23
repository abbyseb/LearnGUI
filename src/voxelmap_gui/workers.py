# workers.py
from __future__ import annotations
from pathlib import Path
from typing import List, Dict, Any, Optional
import re, subprocess, platform
import time

from PyQt6.QtCore import QThread, pyqtSignal

from .backend import start_matlab_generate, _esc_matlab, MATLAB_CODE_DIR, MATLAB_BIN, BASE_DIR
from .preview import (
    load_dicom_volume,
    dicom_file_to_qpixmap,
    triptych_from_volume,
    find_best_ct_series,
    _auto_window, _window_u8, _to_qpixmap
)
from .run_preparation import read_expected_ct_count_from_seriesinfo
import numpy as np

from PyQt6.QtGui import QPixmap


class _PollWatcher(QThread):
    """
    Base class for poll-based watchers with race-free cancel and helpers.
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self._stop_flag = False

    def stop(self):
        self._stop_flag = True
        try:
            self.requestInterruption()
        except Exception:
            pass

    def is_stopped(self) -> bool:
        return self._stop_flag or self.isInterruptionRequested()

    def sleep_ms(self, ms: int) -> bool:
        end = time.monotonic() + max(0.0, ms) / 1000.0
        while not self.is_stopped():
            left = end - time.monotonic()
            if left <= 0:
                return True
            time.sleep(min(0.05, left))
        return False

    def throttle(self, last_emit_ts: float, min_ms: int) -> tuple[bool, float]:
        now = time.monotonic()
        if (now - last_emit_ts) * 1000.0 >= max(0, min_ms):
            return True, now
        return False, last_emit_ts


class MatlabJob(QThread):
    finished_ok = pyqtSignal(str)
    failed = pyqtSignal(str)
    matlab_line_received = pyqtSignal(str)
    
    def __init__(self, rt_ct_pres_list, 
                 skip_copy, skip_dicom2mha, skip_downsample,
                 skip_drr, skip_compress,
                 include_2d_dvf, do_3d_low, do_3d_full,
                 skip_kv_preprocess,
                 work_root, log_file, 
                 mode="Pipeline", start_at="",
                 parent=None):
        super().__init__(parent)
        self.rt_list = rt_ct_pres_list or []
        self.skip_copy = skip_copy
        self.skip_dicom2mha = skip_dicom2mha
        self.skip_downsample = skip_downsample
        self.skip_drr = skip_drr
        self.skip_compress = skip_compress
        self.include_2d_dvf = include_2d_dvf
        self.do_3d_low = do_3d_low
        self.do_3d_full = do_3d_full
        self.skip_kv_preprocess = skip_kv_preprocess
        self.work_root = Path(work_root)
        self.log_file = Path(log_file) if log_file else None
        self.mode = mode
        self.start_at = start_at
        self._process: Optional[subprocess.Popen] = None
        self._cancelled = False

    def run(self):
        try:
            self._process = start_matlab_generate(
                rt_ct_pres_list=self.rt_list,
                work_root=self.work_root,
                skip_copy=self.skip_copy,
                skip_dicom2mha=self.skip_dicom2mha,
                skip_downsample=self.skip_downsample,
                skip_drr=self.skip_drr,
                skip_compress=self.skip_compress,
                include_2d_dvf=self.include_2d_dvf,
                do_3d_low=self.do_3d_low,
                do_3d_full=self.do_3d_full,
                skip_kv_preprocess=self.skip_kv_preprocess,
                log_file=self.log_file,
                mode=self.mode,
                start_at=self.start_at
            )
            
            # Read stdout line-by-line in real-time
            if self._process.stdout:
                for line in iter(self._process.stdout.readline, ''):
                    if self._cancelled:
                        break
                    line = line.strip()
                    if line:
                        self.matlab_line_received.emit(line)

            return_code = self._process.wait()
            
            if self._cancelled:
                return

            if return_code == 0:
                self.finished_ok.emit("MATLAB processing complete.")
            else:
                error_message = f"MATLAB exited with non-zero code: {return_code}."
                self.failed.emit(error_message)

        except Exception as e:
            if not self._cancelled:
                self.failed.emit(f"Failed to launch MATLAB: {str(e)}")
        finally:
            self._process = None

    def cancel(self):
        """Request cancellation."""
        self._cancelled = True
        if self._process and self._process.poll() is None:
            try:
                if platform.system().lower().startswith("win"):
                    subprocess.run(
                        ['taskkill', '/F', '/T', '/PID', str(self._process.pid)],
                        check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                    )
                else:
                    self._process.terminate()
            except Exception:
                pass

    def force_kill(self):
        """Force kill MATLAB."""
        self._cancelled = True
        if self._process and self._process.poll() is None:
            try:
                self._process.kill()
            except Exception:
                pass

