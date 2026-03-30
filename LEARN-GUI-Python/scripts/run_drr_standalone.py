#!/usr/bin/env python3
"""
Standalone DRR check: run DRR on a pipeline run folder.

Usage:
    python scripts/run_drr_standalone.py [path/to/train]

Examples:
    python scripts/run_drr_standalone.py
        -> Uses latest run in Runs/
    python scripts/run_drr_standalone.py Runs/VALKIM_01_Run_20260324_145945/Patient01/train
        -> Uses given train dir
"""
from pathlib import Path
import re
import sys

# Add project root for imports
PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT))


def find_latest_train_dir() -> Path | None:
    """Find train/ in the most recent run folder."""
    runs = PROJECT_ROOT / "Runs"
    if not runs.exists():
        return None
    run_dirs = sorted(
        [d for d in runs.iterdir() if d.is_dir() and "_Run_" in d.name],
        key=lambda p: p.stat().st_mtime,
        reverse=True,
    )
    for run_dir in run_dirs:
        for sub in run_dir.iterdir():
            if sub.is_dir() and (sub / "train").exists():
                return sub / "train"
    return None


def main():
    if len(sys.argv) >= 2:
        train_dir = Path(sys.argv[1]).resolve()
    else:
        train_dir = find_latest_train_dir()
        if train_dir is None:
            print("No run folder found. Usage: python scripts/run_drr_standalone.py [path/to/train]")
            sys.exit(1)
        print(f"Using latest run: {train_dir}")

    if not train_dir.exists():
        print(f"Error: {train_dir} does not exist")
        sys.exit(1)

    from modules.drr_generation.generate import default_geometry_path
    geometry_path = default_geometry_path()
    if not geometry_path.exists():
        print(f"Error: Geometry not found: {geometry_path}")
        sys.exit(1)

    sub_cts = sorted(train_dir.glob("sub_CT_*.mha"))
    if not sub_cts:
        print(f"Error: No sub_CT_*.mha in {train_dir}")
        print("  Run DICOM2MHA and DOWNSAMPLE first.")
        sys.exit(1)

    print(f"Train dir: {train_dir}")
    print(f"Geometry:  {geometry_path}")
    print(f"Found {len(sub_cts)} sub_CT volumes")
    print("Running DRR generation...")
    print()

    from modules.drr_generation.generate import generate_drrs

    n_ok = 0
    for i, mha in enumerate(sub_cts, start=1):
        m = re.search(r"(\d+)", mha.stem)
        ct_num = int(m.group(1)) if m else i
        ok, err = generate_drrs(
            str(mha),
            str(train_dir),
            geometry_file=str(geometry_path),
            ct_num=ct_num,
        )
        if not ok:
            print(f"  FAILED {mha.name}: {err}")
            sys.exit(1)
        n_ok += 1
        print(f"  OK {mha.name} ({n_ok}/{len(sub_cts)})")

    out_count = len(list(train_dir.glob("*_Proj_*.bin")))
    print()
    print(f"Done. Wrote {out_count} projection files to {train_dir}")


if __name__ == "__main__":
    main()
