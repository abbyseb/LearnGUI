"""
DRR generation — pipeline from MATLAB_FUNCTIONALITIES/DRR-GENERATION-PYTHON/forward_project.py

Joseph forward projection (ITK RTK), ConstantImageSource + two-input projector,
uint8 PNG outputs ({ct}_Proj_{###}.png). The COMPRESS step resamples to 128² .bin.
"""
from __future__ import annotations

import re
from pathlib import Path

import itk
import numpy as np
from itk import RTK as rtk

try:
    from PIL import Image
except ImportError:
    Image = None  # type: ignore


def default_geometry_path() -> Path:
    return Path(__file__).resolve().parent / "RTKGeometry_360.xml"


def _load_geometry_xml(path: str):
    reader = rtk.ThreeDCircularProjectionGeometryXMLFileReader.New()
    reader.SetFilename(str(path))
    reader.GenerateOutputInformation()
    try:
        return reader.GetOutputObject()
    except AttributeError:
        return reader.GetOutput()


def _projection_count_from_geometry(geometry) -> int:
    try:
        ga = geometry.GetGantryAngles()
        if hasattr(ga, "Size"):
            n = int(ga.Size())
            if n > 0:
                return n
        n = len(list(ga))
        if n > 0:
            return n
    except Exception:
        pass
    return 0


def _projection_count_from_xml_file(geom_file: Path) -> int:
    try:
        text = geom_file.read_text(encoding="utf-8", errors="ignore")
        return len(re.findall(r"<Projection\b", text))
    except Exception:
        return 0


def _build_circular_geometry(
    sid_mm: float,
    sdd_mm: float,
    first_deg: float,
    step_deg: float,
    n_proj: int,
):
    geom = rtk.ThreeDCircularProjectionGeometry.New()
    n = max(1, int(n_proj))
    for i in range(n):
        angle = float(first_deg) + i * float(step_deg)
        geom.AddProjection(float(sid_mm), float(sdd_mm), angle)
    return geom


def generate_drrs(
    volume_path,
    output_dir,
    geometry_file=None,
    ct_num=1,
    **drr_opts,
):
    """
    Run RTK Joseph forward projection (same wiring as forward_project.py).

    Writes ``{ct_num:02d}_Proj_{idx:03d}.png`` (uint8, min–max normalized per CT).

    Args:
        volume_path: Path to one CT ``.mha``.
        output_dir: train/ directory.
        geometry_file: RTK XML path (overrides ``drr_opts['geometry_path']`` if set).
        ct_num: CT index for output filenames.
        drr_opts: From DRR viewer panel — geometry_source, detector_*, circular_*.

    Returns:
        ``(True, "")`` or ``(False, error_message)``.
    """
    if Image is None:
        return False, "Pillow (PIL) is required to write DRR PNG files."

    out_dir = Path(output_dir).expanduser().resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    vol = Path(volume_path).expanduser().resolve()
    if not vol.is_file():
        return False, f"Volume not found: {vol}"

    gp = geometry_file if geometry_file is not None else drr_opts.get("geometry_path")
    if gp is None:
        gp = default_geometry_path()
    geom_path = Path(gp).expanduser().resolve()

    src = (drr_opts.get("geometry_source") or "xml").strip().lower()

    ox, oy, oz = drr_opts.get("detector_origin", (-200.0, -150.0, 0.0))
    spx, spy, spz = drr_opts.get("detector_spacing", (0.388, 0.388, 1.0))
    nx, ny = drr_opts.get("detector_size_xy", (1024, 768))

    ImageF3 = itk.Image[itk.F, 3]

    try:
        if src in ("xml", "file"):
            if not geom_path.is_file():
                return False, f"Geometry XML not found: {geom_path}"
            geometry = _load_geometry_xml(str(geom_path))
        elif src in ("circular", "orbit", "dynamic"):
            geometry = _build_circular_geometry(
                float(drr_opts.get("circular_sid_mm", 1000.0)),
                float(drr_opts.get("circular_sdd_mm", 1500.0)),
                float(drr_opts.get("circular_first_angle_deg", 0.0)),
                float(drr_opts.get("circular_angle_step_deg", 3.0)),
                int(drr_opts.get("circular_num_projections", 120)),
            )
        else:
            return False, f"Unknown geometry_source: {src!r}"

        num_projections = _projection_count_from_geometry(geometry)
        if num_projections <= 0 and geom_path.is_file():
            num_projections = _projection_count_from_xml_file(geom_path)
        if num_projections <= 0:
            return False, "Could not determine projection count from geometry"

        itk_image = itk.imread(str(vol), itk.F)

        constant_source = rtk.ConstantImageSource[ImageF3].New()
        constant_source.SetOrigin([float(ox), float(oy), float(oz)])
        constant_source.SetSpacing([float(spx), float(spy), float(spz)])
        constant_source.SetSize([int(nx), int(ny), int(num_projections)])
        constant_source.SetConstant(0.0)

        projector = rtk.JosephForwardProjectionImageFilter[ImageF3, ImageF3].New()
        projector.SetInput(0, constant_source.GetOutput())
        projector.SetInput(1, itk_image)
        projector.SetGeometry(geometry)
        projector.Update()

        drr_stack = np.asarray(itk.GetArrayFromImage(projector.GetOutput()))
        if drr_stack.ndim != 3:
            return False, f"Unexpected projection stack shape {drr_stack.shape}"

        if drr_stack.shape[0] != num_projections:
            if drr_stack.shape[-1] == num_projections:
                drr_stack = np.moveaxis(drr_stack, -1, 0)
            elif drr_stack.shape[1] == num_projections:
                drr_stack = np.moveaxis(drr_stack, 1, 0)
            else:
                return False, (
                    f"Stack shape {drr_stack.shape} incompatible with {num_projections} projections"
                )

        dmin = float(np.min(drr_stack))
        dmax = float(np.max(drr_stack))
        denom = (dmax - dmin) if (dmax > dmin) else 1.0
        intensity_u8 = ((drr_stack - dmin) / denom * 255.0).astype(np.uint8)

        ct = int(ct_num)
        for i in range(num_projections):
            frame = intensity_u8[i]
            out_path = out_dir / f"{ct:02d}_Proj_{i + 1:03d}.png"
            Image.fromarray(frame, mode="L").save(out_path)

        return True, ""

    except Exception as e:
        return False, f"{type(e).__name__}: {e}"
        # For now, we save the processed intensity map.
        # itk.imwrite(itk.GetImageFromArray(intensity), hnc_path)
        
        return True
    except Exception as e:
        print(f"Error during HNC conversion: {e}")
        return False

if __name__ == "__main__":
    # Example usage
    # generate_drrs("sub_CT_01.mha", "output_projs")
    pass

