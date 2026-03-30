# downsampling Module

This module is responsible for reducing the resolution of CT volumes to a standardized low-resolution grid for efficient 3D registration.

## Files
- **`downsample.py`**: Contains the core resampling logic using ITK. It processes full-resolution `CT_XX.mha` files into low-resolution `sub_CT_XX.mha` files.

## Methodology
The module implements a specific downsampling strategy to maintain parity with legacy clinical implementations:
1. **Physical Extent Preservation**: The input volume is resampled to a **128x128x128** grid. The spacing is dynamically calculated to ensure the entire physical bounding box of the original CT is covered.
2. **Metadata Reset**: After resampling, the volume's spatial metadata is explicitly reset:
    - **Origin** -> `(0, 0, 0)`
    - **Spacing** -> `(1.0, 1.0, 1.0)`
    - **Direction** -> Identity Matrix
3. **Clinical Rationale**: This "unit-coordinate" reset is a requirement for the DVF generation pipeline. It ensures that the Displacement Vector Fields (DVF) produced by Elastix are relative to a standardized 128-voxel grid, which the GUI viewer uses to overlay results on the planning CT.

## Pipeline Integration
- **Bigger Picture**: This module provides the "Low-Res" branch of the pipeline, enabling fast 3D-to-3D registration for motion estimation.
- **GUI Connection**: Triggered by the "DOWNSAMPLE" step in the clinical workflow. Progress is tracked via per-volume callbacks.
- **Pre-requisites**: Full-resolution `CT_XX.mha` files (usually from `dicom2mha`).
- **Expected Next Step**: `sub_CT_XX.mha` files are used as input for **DVF3D_LOW** generation.
