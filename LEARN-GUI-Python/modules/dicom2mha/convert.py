"""
DICOM to MHA matching MATLAB DicomToMha_VALKIM.m, ConvertDCMVolumesToMHA.m, DICOM2IEC.m.

Gated phases: valkim.try_valkim_volumes then mha_write.write_mha_float32.
Otherwise: dicom2iec_volume, apply_water_attenuation, write_mha_float32 per series.
"""
from __future__ import annotations

import pydicom
from pathlib import Path
from typing import Callable, Dict, List, Optional

from .dicom2iec import _iop6, apply_water_attenuation, dicom2iec_volume
from .mha_write import write_mha_float32
from .valkim import CTRecord, try_valkim_volumes


def _dicom_description_to_phase_bin(descript: str) -> Optional[int]:
    """
    MATLAB dicomDscrptToPhaseBin.m: NaN when not gated; Python uses None.
    Maps Gated 50% to 6, Gated 0% to 1, etc.
    """
    if not descript or "Gated" not in descript:
        return None
    try:
        after_gated = descript.split("Gated")[1]
        num_str = after_gated.split("%")[0].strip()
        if "," in num_str:
            num_str = num_str.split(",")[-1].strip()
        val = float(num_str)
        return int(round(val / 10.0) + 1)
    except (ValueError, IndexError):
        return None


def _is_dicom_file(path: Path) -> bool:
    try:
        with open(path, "rb") as fh:
            fh.seek(128)
            return fh.read(4) == b"DICM"
    except OSError:
        return False


def _ipp_z(ds) -> float:
    ipp = getattr(ds, "ImagePositionPatient", None)
    if ipp is not None and len(ipp) >= 3:
        try:
            return float(ipp[2])
        except (TypeError, ValueError):
            pass
    sl = getattr(ds, "SliceLocation", None)
    if sl is not None:
        try:
            return float(sl)
        except (TypeError, ValueError):
            pass
    return 0.0


def _scan_ct_records(dicom_directory: Path) -> List[Path]:
    """
    Collect CT DICOM paths under directory (recursive). Paths sorted for stable output.
    Requires standard DICOM preamble (DICM at offset 128), same as a typical file copy.
    """
    paths: List[Path] = []
    for f in dicom_directory.rglob("*"):
        if not f.is_file():
            continue
        if not _is_dicom_file(f):
            continue
        try:
            ds = pydicom.dcmread(str(f), stop_before_pixels=True, force=True)
        except Exception:
            continue
        if getattr(ds, "Modality", "") != "CT":
            continue
        paths.append(f)
    paths.sort(key=lambda p: str(p).lower())
    return paths


def _build_meta(ct_ordered: List[Path]) -> Dict[Path, CTRecord]:
    meta: Dict[Path, CTRecord] = {}
    for p in ct_ordered:
        ds = pydicom.dcmread(str(p), stop_before_pixels=True, force=True)
        desc = str(getattr(ds, "SeriesDescription", "") or "")
        phase = _dicom_description_to_phase_bin(desc)
        uid = str(getattr(ds, "SeriesInstanceUID", "") or "")
        rows = int(getattr(ds, "Rows", 0) or 0)
        cols = int(getattr(ds, "Columns", 0) or 0)
        std = str(getattr(ds, "StudyDescription", "") or "")
        srd = str(getattr(ds, "SeriesDescription", "") or "")
        meta[p] = CTRecord(
            path=p,
            desc=desc,
            phase=phase,
            series_uid=uid,
            ipp_z=_ipp_z(ds),
            iop=_iop6(ds),
            rows=rows,
            cols=cols,
            study_description=std,
            series_description=srd,
        )
    return meta


def _series_order(ct_ordered: List[Path], meta: Dict[Path, CTRecord]) -> List[str]:
    """First occurrence order of SeriesInstanceUID (MATLAB unique with stable order)."""
    out: List[str] = []
    seen = set()
    for p in ct_ordered:
        u = meta[p].series_uid
        if u not in seen:
            seen.add(u)
            out.append(u)
    return out


def _series_info_line_from_record(rec: CTRecord) -> str:
    has_std = rec.study_description.strip() != ""
    has_srd = rec.series_description.strip() != ""
    if has_std and has_srd:
        return f"{rec.study_description}_{rec.series_description}"
    if not has_std and has_srd:
        return rec.series_description
    if has_std and not has_srd:
        return rec.study_description
    return "No description fields in DICOM file"


def convert_dicom_to_mha(
    dicom_directory,
    output_base_path,
    *,
    on_status: Optional[Callable[[str], None]] = None,
    on_phase_done: Optional[Callable[[int, int, str], None]] = None,
):
    """
    MATLAB-equivalent conversion (pydicom + numpy; no ITK ImageSeriesReader).

    Gated SeriesDescription: VALKIM path (try_valkim_volumes) then write_mha_float32.
    Else: per-series dicom2iec_volume, apply_water_attenuation, write_mha_float32.

    on_phase_done(done_index, total_phases, filename) fires after each CT_XX.mha.
    """

    def _status(msg: str) -> None:
        if on_status:
            on_status(msg)
        else:
            print(msg)

    dicom_path = Path(dicom_directory)
    if not dicom_path.exists():
        _status(f"Error: Directory {dicom_directory} does not exist.")
        return []

    obp = Path(output_base_path)
    output_dir = obp if obp.is_dir() else obp.parent
    output_dir.mkdir(parents=True, exist_ok=True)

    water_att = 0.013

    _status(f"Scanning {dicom_directory} for CT DICOM...")
    ct_ordered = _scan_ct_records(dicom_path)
    if not ct_ordered:
        _status("No CT DICOM files found.")
        return []

    meta = _build_meta(ct_ordered)
    generated_files: List[str] = []
    series_info_lines: List[str] = []

    valkim_result = try_valkim_volumes(ct_ordered, meta, water_att)
    if valkim_result is not None:
        _status("Using VALKIM gated-phase path (MATLAB DicomToMha_VALKIM).")
        valkim_volumes = sorted(valkim_result, key=lambda t: t[0])
        n_out = len(valkim_volumes)
        for j, (nbin, vol, pix, line) in enumerate(valkim_volumes, start=1):
            name = f"CT_{nbin:02d}.mha"
            target = output_dir / name
            _status(f"  Writing {name} (phase bin {nbin}, shape {vol.shape})...")
            try:
                write_mha_float32(vol, pix, target)
                generated_files.append(str(target))
                series_info_lines.append(f"CT_{nbin:02d}: {line}")
                if on_phase_done:
                    on_phase_done(j, n_out, name)
            except Exception as e:
                _status(f"  Error writing {name}: {e}")
    else:
        _status("Using per-series DICOM2IEC path (MATLAB ConvertDCMVolumesToMHA).")
        uids = _series_order(ct_ordered, meta)
        n_out = len(uids)
        for j, uid in enumerate(uids, start=1):
            files = [p for p in ct_ordered if meta[p].series_uid == uid]
            if not files:
                continue
            name = f"CT_{j:02d}.mha"
            target = output_dir / name
            _status(f"  Series {j}/{n_out} ({len(files)} files) -> {name}...")
            try:
                vol, pix = dicom2iec_volume(files)
                apply_water_attenuation(vol, water_att)
                write_mha_float32(vol, pix, target)
                generated_files.append(str(target))
                series_info_lines.append(
                    f"CT_{j:02d}: {_series_info_line_from_record(meta[files[0]])}"
                )
                if on_phase_done:
                    on_phase_done(j, n_out, name)
            except Exception as e:
                _status(f"  Error processing series {j}: {e}")

    if series_info_lines:
        try:
            with open(output_dir / "SeriesInfo.txt", "w", encoding="utf-8") as f:
                f.write("\n".join(series_info_lines))
        except Exception as e:
            _status(f"Warning: could not write SeriesInfo.txt: {e}")

    return generated_files

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 2:
        convert_dicom_to_mha(sys.argv[1], sys.argv[2])
    else:
        print("Usage: convert.py <dicom_dir> <output_mha_sample_path>")
