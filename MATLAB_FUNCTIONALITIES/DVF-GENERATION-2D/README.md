# 2D Displacement Vector Field (DVF) Generation

This component calculates the 2D projected motion (displacement) associated with each DRR projection, providing the ground truth for training 2D-3D registration or motion estimation models.

## Purpose
- Generate 2D vector fields that describe how 3D motion is projected into the 2D imaging plane.
- Associate each DRR frame with its specific anatomical displacement compared to a reference phase (e.g., end-exhalation).

## Key Scripts
- **`getVALKIM2DDVF.m`**: Standard script for generating 2D DVFs for the VALKIM trial. (Note: Ensure this script is present in this directory to enable this step in the GUI).

## Outputs
- `XX_2DDVF_YY.mat` or `.bin`: 2D Displacement Vector Field for phase XX at angle YY.
