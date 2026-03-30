"""
DicomToMha_VALKIM.m — gated phases, IEC geometry, MHA headers.

Fixes MATLAB typo ``imgStacks{n}`` → ``imgStacks{nbin}`` on the IOP(1) flip line.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple

import numpy as np

from .dicom2iec import apply_water_attenuation, _iop6


@dataclass
class CTRecord:
    path: Path
    desc: str
    phase: Optional[int]
    series_uid: str
    ipp_z: float
    iop: np.ndarray
    rows: int
    cols: int
    study_description: str
    series_description: str


def _hu_slice(ds) -> np.ndarray:
    arr = ds.pixel_array.astype(np.float32)
    slope = float(getattr(ds, "RescaleSlope", 1.0))
    intercept = float(getattr(ds, "RescaleIntercept", 0.0))
    return arr * slope + intercept


def _series_info_line(ds_first) -> str:
    std = getattr(ds_first, "StudyDescription", None)
    srd = getattr(ds_first, "SeriesDescription", None)
    has_std = std is not None and str(std).strip() != ""
    has_srd = srd is not None and str(srd).strip() != ""
    if has_std and has_srd:
        return f"{std}_{srd}"
    if not has_std and has_srd:
        return str(srd)
    if has_std and not has_srd:
        return str(std)
    return "No description fields in DICOM file"


def _stack_slots_for_bin(zs: List[float], iop5: float) -> np.ndarray:
    """Per-file stack index in ctList order within this bin (0 .. n-1)."""
    n = len(zs)
    if n == 0:
        return np.zeros(0, dtype=int)
    ar = np.array(zs, dtype=np.float64)
    if iop5 > 0:
        order = np.argsort(-ar, kind="stable")
    else:
        order = np.argsort(ar, kind="stable")
    inv = np.empty(n, dtype=int)
    inv[order] = np.arange(n, dtype=int)
    return inv


def try_valkim_volumes(
    ct_ordered: List[Path],
    meta: Dict[Path, CTRecord],
    water_att: float,
) -> Optional[List[Tuple[int, np.ndarray, np.ndarray, str]]]:
    """
    If gated phases exist, build IEC volumes like VALKIM.

    Returns:
      None → use per-series DICOM2IEC (ConvertDCMVolumesToMHA).
      Else list of (phase_bin_index, volume, pixel_dims, series_info_line).
    """
    import pydicom

    if not ct_ordered:
        return None

    bins = sorted({meta[p].phase for p in ct_ordered if meta[p].phase is not None})
    if not bins:
        return None

    files_per: Dict[int, List[Path]] = {b: [] for b in bins}
    zs_per: Dict[int, List[float]] = {b: [] for b in bins}
    for p in ct_ordered:
        ph = meta[p].phase
        if ph is None:
            continue
        files_per[ph].append(p)
        zs_per[ph].append(meta[p].ipp_z)

    slots: Dict[int, np.ndarray] = {}
    for b in bins:
        paths_b = files_per[b]
        if not paths_b:
            return None
        iop5 = float(meta[paths_b[0]].iop[4])
        slots[b] = _stack_slots_for_bin(zs_per[b], iop5)

    img_stacks: Dict[int, np.ndarray] = {}
    bin_local_i: Dict[int, int] = {b: 0 for b in bins}

    for p in ct_ordered:
        ph = meta[p].phase
        if ph is None:
            continue
        slot_list = slots[ph]
        li = bin_local_i[ph]
        if li >= len(slot_list):
            return None
        stack_i = int(slot_list[li])
        bin_local_i[ph] = li + 1

        ds = pydicom.dcmread(str(p), force=True)
        plane = _hu_slice(ds)
        r, c = plane.shape
        if ph not in img_stacks:
            nsl = len(files_per[ph])
            img_stacks[ph] = np.zeros((r, c, nsl), dtype=np.float32)
        stk = img_stacks[ph]
        if stk.shape[0] != r or stk.shape[1] != c:
            return None
        stk[:, :, stack_i] = plane

    for b in bins:
        if bin_local_i[b] != len(files_per[b]):
            return None

    # MATLAB: after permute [2,3,1], size(s,2) is slice count; same as shape[2] before permute.
    chk = [img_stacks[b].shape[2] for b in bins]
    if len(set(chk)) > 1:
        return None

    out: List[Tuple[int, np.ndarray, np.ndarray, str]] = []
    for nbin in sorted(bins):
        M = img_stacks[nbin]
        ds0 = pydicom.dcmread(str(files_per[nbin][0]), force=True)
        iop = _iop6(ds0)

        M = np.transpose(M, (1, 2, 0))
        if iop[0] < 0:
            M = M[::-1, :, :]
        M = M[:, :, ::-1]

        apply_water_attenuation(M, water_att)

        ps = getattr(ds0, "PixelSpacing", None)
        if ps is None or len(ps) < 2:
            return None
        st = getattr(ds0, "SliceThickness", None)
        if st is None:
            st = getattr(ds0, "SpacingBetweenSlices", None)
        if st is None:
            st = float(ps[0])
        st = float(st)
        pix = np.array([float(ps[0]), st, float(ps[1])], dtype=np.float64)
        line = _series_info_line(ds0)
        out.append((nbin, M, pix, line))

    out.sort(key=lambda t: t[0])
    return out
