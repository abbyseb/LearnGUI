# 3D Displacement Vector Field (DVF) Generation (Full Resolution)

This component calculates the high-resolution 3D anatomical motion (displacement) between CT phases.

## Purpose
- Provide the ultimate ground-truth motion models for clinical-grade registration and dose calculation.
- High-fidelity anatomical mapping between respiratory phases.
- Typically used on the original, non-downsampled CT volumes.

## Key Scripts
- **`VALKIM3DDVF_full.m`**: Main logic for calculating full-resolution 3D displacement vector fields. (Note: Ensure this script is present in this directory to enable this step in the GUI).

## Outputs
- `DVF_XX.mha`: Full-resolution 3D Displacement Vector Field for phase XX.
