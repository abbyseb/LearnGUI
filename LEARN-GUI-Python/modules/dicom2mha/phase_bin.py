"""dicomDscrptToPhaseBin.m — gated series description → phase bin index."""

from __future__ import annotations

from typing import Optional


def phase_bin_from_description(descript: str) -> Optional[int]:
    if not descript or "Gated" not in descript:
        return None
    parts = descript.split("Gated", 1)
    if len(parts) < 2:
        return None
    pct_part = parts[1].split("%", 1)[0].strip()
    if "," in pct_part:
        pct_part = pct_part.split(",")[-1].strip()
    try:
        pct = float(pct_part)
        return int(round(pct / 10.0) + 1)
    except ValueError:
        return None
