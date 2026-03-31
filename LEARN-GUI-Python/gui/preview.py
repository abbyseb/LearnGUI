# preview.py
from __future__ import annotations
from pathlib import Path
from typing import Tuple, List, Optional, Iterable
import numpy as np

try:
    import SimpleITK as sitk
except Exception:
    sitk = None

try:
    import pydicom
    from pydicom.misc import is_dicom
except Exception:
    pydicom = None
    def is_dicom(_): return False

from PyQt6.QtGui import QImage, QPixmap


# --------- Windowing helpers ---------
def _window_u8(arr: np.ndarray, wl: float, ww: float) -> np.ndarray:
    lo = wl - ww / 2.0
    hi = wl + ww / 2.0
    arr = np.clip(arr, lo, hi)
    rng = max(hi - lo, 1e-6)
    arr = ((arr - lo) / rng) * 255.0
    return np.clip(arr, 0, 255).astype(np.uint8)

def _auto_window(arr: np.ndarray) -> Tuple[float, float]:
    a = arr[np.isfinite(arr)]
    if a.size < 10:
        median = np.nanmedian(a) if a.size > 0 else 0
        spread = (np.nanmax(a) - np.nanmin(a)) if a.size > 1 else 1.0
        return (median, max(spread, 1.0))
        
    p5, p95 = np.percentile(a, [5, 95])
    ww = max(p95 - p5, 1.0)
    wl = (p95 + p5) / 2.0
    return wl, ww


def _to_qpixmap(u8: np.ndarray) -> QPixmap:
    """Map a uint8 2D array to a QPixmap (grayscale)."""
    if u8 is None:
        return QPixmap()
    if u8.dtype != np.uint8:
        u8 = u8.astype(np.uint8, copy=False)
    if not u8.flags['C_CONTIGUOUS']:
        u8 = np.ascontiguousarray(u8)

    h, w = int(u8.shape[0]), int(u8.shape[1])
    bpl = int(u8.strides[0])

    qimg = QImage(u8.tobytes(), w, h, bpl, QImage.Format.Format_Grayscale8)
    qimg = qimg.copy()
    return QPixmap.fromImage(qimg)


def dicom_file_to_qpixmap(path: Path,
                          window: Tuple[float, float] = (-600.0, 1500.0),
                          allow_auto: bool = True) -> QPixmap:
    """Render a single DICOM file to a QPixmap (axial)."""
    if pydicom is None:
        raise RuntimeError("pydicom not available (pip install pydicom)")
    ds = pydicom.dcmread(str(path), force=True)
    sl = _load_dicom_slice(ds)
    wl, ww = window
    u8 = _window_u8(sl, wl, ww)
    if allow_auto and (u8.max() - u8.min() < 10):
        wl2, ww2 = _auto_window(sl)
        u8 = _window_u8(sl, wl2, ww2)
    return _to_qpixmap(u8)


def read_mha_volume(path: Path) -> tuple[
    np.ndarray,
    tuple[float, float, float],   # spacing (z,y,x)
    tuple[float, float, float],   # origin (x,y,z) from ITK
    np.ndarray,                   # 3x3 direction matrix
    object                        # SimpleITK image (for geometry-accurate resampling)
]:
    """
    Read MHA volume and return array + metadata + original sitk image.
    
    Returns:
        (volume, spacing, origin, direction, sitk_image)
        
    The sitk_image is the original file's image object, preserving exact geometry
    for accurate overlay resampling.
    """
    if sitk is None:
        raise RuntimeError("SimpleITK not available (pip install SimpleITK)")
    
    import time
    last_exception = None
    for _ in range(5):
        try:
            img = sitk.ReadImage(str(path))
            
            vol = sitk.GetArrayFromImage(img).astype(np.float32)
            spx = img.GetSpacing()
            spacing = (float(spx[2]), float(spx[1]), float(spx[0]))
            origin_xyz = tuple(float(v) for v in img.GetOrigin())
            direction = np.array(img.GetDirection(), dtype=float).reshape(3, 3)

            # Return the original sitk image for geometry-accurate overlay resampling
            return vol, spacing, origin_xyz, direction, img

        except RuntimeError as e:
            last_exception = e
            time.sleep(0.2)

    return None # Return None instead of raising, let caller handle it.


# --------- DICOM helpers ---------
def _is_ct(ds) -> bool:
    try:
        return str(ds.Modality).upper() == "CT"
    except Exception:
        return False

def _load_dicom_slice(ds) -> np.ndarray:
    arr = ds.pixel_array.astype(np.float32)
    slope = float(getattr(ds, "RescaleSlope", 1.0) or 1.0)
    inter = float(getattr(ds, "RescaleIntercept", 0.0) or 0.0)
    return arr * slope + inter

def _series_instance_uid(ds) -> Optional[str]:
    return getattr(ds, "SeriesInstanceUID", None)

def _instance_number(ds) -> int:
    try:
        return int(getattr(ds, "InstanceNumber", 0))
    except Exception:
        return 0

def find_best_ct_series(train_root: Path) -> Optional[List[Path]]:
    """Scan for DICOM files, group by SeriesInstanceUID, choose largest CT series."""
    if pydicom is None:
        return None
    dcm_files: List[Path] = [p for p in train_root.rglob("*") if p.is_file() and p.suffix.lower() in (".dcm", "")]
    candidates = []
    for p in dcm_files:
        try:
            if not is_dicom(str(p)):
                continue
            ds = pydicom.dcmread(str(p), stop_before_pixels=True, force=True)
            if not _is_ct(ds):
                continue
            uid = _series_instance_uid(ds)
            if not uid:
                continue
            candidates.append((uid, p))
        except Exception:
            continue
    if not candidates:
        return None

    from collections import defaultdict
    groups = defaultdict(list)
    for uid, p in candidates:
        groups[uid].append(p)

    best_uid = max(groups.keys(), key=lambda u: len(groups[u]))
    files = groups[best_uid]

    try:
        files_meta = []
        for p in files:
            ds = pydicom.dcmread(str(p), stop_before_pixels=True, force=True)
            files_meta.append((p, _instance_number(ds)))
        files = [p for p, _ in sorted(files_meta, key=lambda t: t[1])]
    except Exception:
        files = sorted(files)

    return files


def _vol_triptych(vol: np.ndarray, axial_index: int | None,
                  wl: float, ww: float, allow_auto: bool) -> tuple[QPixmap, QPixmap, QPixmap]:
    """vol: (z,y,x) float32 HU"""
    z, y, x = vol.shape
    ia = z // 2 if axial_index is None else max(0, min(axial_index, z - 1))
    ic = y // 2
    isg = x // 2
    ax = vol[ia]
    co = vol[:, ic, :]
    sa = vol[:, :, isg]

    def _map(u):
        u8 = _window_u8(u, wl, ww)
        if allow_auto and (u8.max() - u8.min() < 10):
            wl2, ww2 = _auto_window(u.astype(np.float32))
            u8 = _window_u8(u, wl2, ww2)
        return _to_qpixmap(u8)

    return _map(ax), _map(co), _map(sa)

def load_dicom_volume(series_files: Iterable[Path]) -> np.ndarray:
    """Load a DICOM CT series into (z,y,x) HU float32."""
    if pydicom is None:
        raise RuntimeError("pydicom not available")
    files = list(series_files)
    metas = []
    for p in files:
        try:
            ds = pydicom.dcmread(str(p), force=True)
            metas.append((p, _instance_number(ds), ds))
        except Exception:
            continue
    if not metas:
        raise RuntimeError("No readable DICOM slices")
    metas.sort(key=lambda t: t[1])
    arrs = []
    for _, _, ds in metas:
        arrs.append(_load_dicom_slice(ds))
    vol = np.stack(arrs, axis=0).astype(np.float32)
    return vol


def triptych_from_volume(vol: np.ndarray, axial_index: int | None = None,
                         window: Tuple[float, float] = (-600.0, 1500.0),
                         allow_auto: bool = True) -> tuple[QPixmap, QPixmap, QPixmap]:
    wl, ww = window
    return _vol_triptych(vol, axial_index, wl, ww, allow_auto)