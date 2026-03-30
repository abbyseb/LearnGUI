"""Build RTK ThreeDCircularProjectionGeometry from XML or circular parameters."""
from __future__ import annotations

from pathlib import Path


def load_geometry_from_xml(path: Path):
    import itk

    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError(f"Geometry XML not found: {path}")
    rtk = itk.RTK
    reader = rtk.ThreeDCircularProjectionGeometryXMLFileReader.New()
    reader.SetFilename(str(path))
    reader.GenerateOutputInformation()
    return reader.GetOutputObject()


def build_circular_geometry(
    sid_mm: float,
    sdd_mm: float,
    gantry_angles_deg: list[float],
):
    """
    Ideal circular orbit: one projection per angle via RTK AddProjection(sid, sdd, gantry_deg).

    This matches the *intent* of RTKGeometry_360.xml (1000 / 1500 mm, 3° steps) but not
    byte-identical matrices; use XML mode to match the shipped file exactly.
    """
    import itk

    rtk = itk.RTK
    geom = rtk.ThreeDCircularProjectionGeometry.New()
    for a in gantry_angles_deg:
        geom.AddProjection(float(sid_mm), float(sdd_mm), float(a))
    return geom


def make_geometry(
    *,
    source: str,
    geometry_path: Path | None,
    circular_sid: float,
    circular_sdd: float,
    circular_first_angle_deg: float,
    circular_angle_step_deg: float,
    circular_num_projections: int,
):
    source = (source or "xml").strip().lower()
    if source in ("xml", "file"):
        return load_geometry_from_xml(Path(geometry_path))
    if source in ("circular", "orbit", "dynamic"):
        n = max(1, int(circular_num_projections))
        angles = [
            float(circular_first_angle_deg + i * float(circular_angle_step_deg))
            for i in range(n)
        ]
        return build_circular_geometry(
            float(circular_sid), float(circular_sdd), angles
        )
    raise ValueError(f"Unknown geometry source: {source!r}")
