"""Off-GUI-thread DVF / MHA work so the Qt event loop stays responsive."""
from __future__ import annotations

from pathlib import Path
from typing import Any

import numpy as np
from PyQt6.QtCore import QThread, pyqtSignal


def _sitk_dvf_magnitude(img) -> Any:
    import SimpleITK as sitk

    comps = int(img.GetNumberOfComponentsPerPixel()) if hasattr(img, "GetNumberOfComponentsPerPixel") else 1
    if comps > 1:
        try:
            mag = sitk.VectorMagnitude(img)
        except Exception:
            chans = [sitk.VectorIndexSelectionCast(img, c) for c in range(comps)]
            mag = sitk.Sqrt(sum([sitk.Square(c) for c in chans]))
    else:
        mag = sitk.Cast(img, sitk.sitkFloat32)
    return sitk.Cast(mag, sitk.sitkFloat32)


def dvf_file_max_magnitude(path: str) -> float:
    import SimpleITK as sitk

    p = Path(path)
    if not p.is_file():
        return 0.0
    img = sitk.ReadImage(str(p))
    mag = _sitk_dvf_magnitude(img)
    arr = sitk.GetArrayViewFromImage(mag)
    return float(np.nanmax(np.asarray(arr))) if arr.size else 0.0


def load_dvf_overlay_log_volume(
    dvf_path: str,
    base_mha_path: str,
    sar_reorient_params: dict[str, Any],
    perm_phys_to_log: tuple[int, int, int] | None,
) -> np.ndarray:
    """Same geometry as DVFPanel synchronous path: resample to base CT, SAR, then logical perm."""
    import SimpleITK as sitk

    base = sitk.ReadImage(str(base_mha_path))
    img = sitk.ReadImage(str(dvf_path))
    mag = _sitk_dvf_magnitude(img)
    mag = sitk.Resample(
        mag,
        base,
        sitk.Transform(),
        sitk.sitkLinear,
        0.0,
        mag.GetPixelID(),
    )
    arr_zyx = sitk.GetArrayFromImage(mag).astype(np.float32)
    perm = sar_reorient_params["perm_zyx_to_SAR"]
    flips = sar_reorient_params["flips_SAR"]
    vol_SAR = np.transpose(arr_zyx, perm)
    if flips[0]:
        vol_SAR = vol_SAR[::-1, :, :]
    if flips[1]:
        vol_SAR = vol_SAR[:, ::-1, :]
    if flips[2]:
        vol_SAR = vol_SAR[:, :, ::-1]
    if perm_phys_to_log is None:
        ov_log = vol_SAR
    else:
        ov_log = np.transpose(vol_SAR, perm_phys_to_log)
    return ov_log.astype(np.float32)


class MhaVolumeLoadThread(QThread):
    """Load train CT MHA for the DVF viewer (read_mha_volume) off the GUI thread."""

    volume_ready = pyqtSignal(int, object, object, object, object, object)
    load_failed = pyqtSignal(int, str, str)

    def __init__(self, path: Path, job_id: int, parent=None):
        super().__init__(parent)
        self._path = path
        self._job_id = job_id

    def run(self):
        from .preview import read_mha_volume

        try:
            if self.isInterruptionRequested():
                return
            vol, spacing, origin, direction, _sitk = read_mha_volume(self._path)
            if self.isInterruptionRequested():
                return
            # Copy direction so the signal payload is safe across threads
            direction = np.array(direction, dtype=np.float64, copy=True)
            vol = np.ascontiguousarray(vol.astype(np.float32, copy=False))
            self.volume_ready.emit(
                self._job_id,
                str(self._path.resolve()),
                vol,
                spacing,
                origin,
                direction,
            )
        except Exception as e:
            self.load_failed.emit(self._job_id, str(self._path), str(e))


class DvfVmaxBatchThread(QThread):
    """Compute |DVF| vmax over all listed files; merge with mtime/size cache."""

    vmax_ready = pyqtSignal(int, object, float)

    def __init__(
        self,
        job_id: int,
        path_strings: list[str],
        existing_cache: dict[str, tuple[int, int, float]],
        parent=None,
    ):
        super().__init__(parent)
        self._job_id = job_id
        self._paths = path_strings
        self._existing = dict(existing_cache)

    def run(self):
        active_keys: set[str] = set()
        merged: dict[str, tuple[int, int, float]] = {}
        global_max = 0.0

        for path_str in self._paths:
            if self.isInterruptionRequested():
                return
            try:
                path = Path(path_str)
                if not path.is_file():
                    continue
                key = str(path.resolve())
                active_keys.add(key)
                st = path.stat()
                sig = (st.st_mtime_ns, st.st_size)
                cached = self._existing.get(key)
                if cached is not None and cached[0] == sig[0] and cached[1] == sig[1]:
                    merged[key] = cached
                    global_max = max(global_max, cached[2])
                    continue
                m = dvf_file_max_magnitude(str(path))
                merged[key] = (sig[0], sig[1], m)
                global_max = max(global_max, m)
            except Exception:
                continue

        for k, v in self._existing.items():
            if k in active_keys and k not in merged:
                merged[k] = v
                global_max = max(global_max, v[2])

        if self.isInterruptionRequested():
            return
        self.vmax_ready.emit(self._job_id, merged, global_max)


class DvfOverlayLoadThread(QThread):
    """Load one DVF, resample to base CT, return logical magnitude volume."""

    overlay_ready = pyqtSignal(int, str, object)
    overlay_failed = pyqtSignal(int, str, str)

    def __init__(
        self,
        gen: int,
        key: str,
        dvf_path: str,
        base_mha_path: str,
        sar_reorient_params: dict[str, Any],
        perm_phys_to_log: tuple[int, int, int] | None,
        parent=None,
    ):
        super().__init__(parent)
        self._gen = gen
        self._key = key
        self._dvf_path = dvf_path
        self._base_mha_path = base_mha_path
        self._sar = sar_reorient_params
        self._perm = perm_phys_to_log

    def run(self):
        try:
            if self.isInterruptionRequested():
                return
            arr = load_dvf_overlay_log_volume(
                self._dvf_path,
                self._base_mha_path,
                self._sar,
                self._perm,
            )
            if self.isInterruptionRequested():
                return
            self.overlay_ready.emit(self._gen, self._key, np.ascontiguousarray(arr))
        except Exception as e:
            self.overlay_failed.emit(self._gen, self._key, str(e))
