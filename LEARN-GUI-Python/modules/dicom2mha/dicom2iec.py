"""
DICOM2IEC.m — DICOM CT series → IEC-style voxel array + PixelDimensions for MHA.

Not IEC 61217 RT geometry: no explicit 3×3 patient matrix for beams/couch; voxel permutes only.
"""

from __future__ import annotations

from pathlib import Path
from typing import List, Tuple

import numpy as np


def _iop6(ds) -> np.ndarray:
    iop = getattr(ds, "ImageOrientationPatient", None)
    if iop is None or len(iop) < 6:
        return np.array([1.0, 0.0, 0.0, 0.0, 1.0, 0.0], dtype=np.float64)
    return np.array([float(x) for x in iop[:6]], dtype=np.float64)


def _slice_location_matlab(ds) -> float:
    """MATLAB uses SliceLocation; fall back to IPP(3) if absent."""
    sl = getattr(ds, "SliceLocation", None)
    if sl is not None:
        try:
            return float(sl)
        except (TypeError, ValueError):
            pass
    ipp = getattr(ds, "ImagePositionPatient", None)
    if ipp is not None and len(ipp) >= 3:
        try:
            return float(ipp[2])
        except (TypeError, ValueError):
            pass
    return 0.0


def _hu_slice(ds) -> np.ndarray:
    arr = ds.pixel_array.astype(np.float32)
    slope = float(getattr(ds, "RescaleSlope", 1.0))
    intercept = float(getattr(ds, "RescaleIntercept", 0.0))
    return arr * slope + intercept


def dicom2iec_volume(file_paths: List[Path]) -> Tuple[np.ndarray, np.ndarray]:
    """
    Port of DICOM2IEC.m (excluding water attenuation and MhaWrite).

    ``file_paths`` order is the MATLAB ``dicomList`` order within one series
    (``ConvertDCMVolumesToMHA`` group order), not sorted by path.
    """
    import pydicom

    if not file_paths:
        raise ValueError("empty DICOM list")

    headers: list = []
    planes: list = []
    sls: list[float] = []
    for p in file_paths:
        ds = pydicom.dcmread(str(p), force=True)
        headers.append(ds)
        planes.append(_hu_slice(ds))
        sls.append(_slice_location_matlab(ds))

    M = np.stack(planes, axis=-1).astype(np.float32)
    M = np.transpose(M, (0, 2, 1))

    iop = _iop6(headers[0])
    sign_tm = np.sign(iop[iop != 0])
    s1 = float(sign_tm[0]) if len(sign_tm) >= 1 else 1.0
    s2 = float(sign_tm[1]) if len(sign_tm) >= 2 else 1.0

    if abs(iop[0]) > 1e-9:
        if s1 < 0:
            M = M[:, :, ::-1]
        if s2 > 0:
            M = M[::-1, :, :]
    else:
        if s1 < 0:
            M = M[::-1, :, :]
        if s2 > 0:
            M = M[:, :, ::-1]

    sl = np.array(sls, dtype=np.float64)
    _, ia = np.unique(sl, return_index=True)
    order = ia[::-1]
    M = M[:, order, :]

    if abs(iop[0]) > 1e-9:
        M = np.transpose(M, (2, 1, 0))

    ps = getattr(headers[0], "PixelSpacing", None)
    if ps is None or len(ps) < 2:
        raise ValueError("PixelSpacing missing or invalid")
    st = getattr(headers[0], "SliceThickness", None)
    if st is None:
        st = getattr(headers[0], "SpacingBetweenSlices", None)
    if st is None:
        st = float(ps[0])
    st = float(st)
    pix = np.array([float(ps[0]), st, float(ps[1])], dtype=np.float64)
    return M, pix


def apply_water_attenuation(M: np.ndarray, water_att: float) -> None:
    if np.isnan(water_att):
        return
    if M.min() >= 0:
        M[:] = M * (water_att / 1000.0)
    else:
        M[:] = (M + 1000.0) * (water_att / 1000.0)
