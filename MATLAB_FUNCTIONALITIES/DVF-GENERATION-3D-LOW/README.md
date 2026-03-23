# 3D Displacement Vector Field (DVF) Generation (Low Resolution)

This component calculates the 3D anatomical motion (displacement) between different CT phases at a reduced resolution for efficiency.

## Purpose
- Generate 3D ground truth motion fields for 3D image registration tasks.
- Provide a coarse anatomical motion model that is less computationally intensive than full-resolution fields.
- Typically used to register downsampled CT volumes (e.g., 128x128x128).

## Key Scripts
- **`getVALKIM3DDVF.m`**: Standard script for generating 3D DVFs for the VALKIM trial. Calculates displacement between each phase and the reference phase.

## Outputs
- `DVF_sub_XX.mha`: Downsampled 3D Displacement Vector Field for phase XX.
