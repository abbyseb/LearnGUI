# drr_compression Module

This module handles the post-processing and compression of Digitally Reconstructed Radiographs (DRRs) into a compact binary format for clinical tracking.

## Files
- **`compress.py`**: The core processing script. It converts image projections (PNG, TIF, TIFF) into normalized, downsampled binary flat-files (`.bin`).

## Methodology
The compression process aligns the Python output with the legacy MATLAB `compressProj.m` methodology:
1. **Log-Domain Conversion**: Projections are converted to the log-domain using specific intensity-scaling formulas:
    - **PNG (8-bit)**: $\log(256 / (P + 1))$
    - **TIF (16-bit)**: $\log(65536 / (P + 1))$
2. **Resampling**: Each projection is zoomed (using linear interpolation) to a standardized **128x128** grid.
3. **Binary Serialization**: The resulting float32 array is flattened in **column-major (Fortran)** order and saved as a raw `.bin` file.
4. **Cleanup**: Original image files are unlinked (deleted) after successful compression to save disk space and simplify the directory structure.

## Pipeline Integration
- **Bigger Picture**: This module transforms raw visual projections into the numerical input required by the clinical tracking and DVFT-net training algorithms.
- **GUI Connection**: Triggered after the "DRR GENERATION" step. Progress is reported in batches to keep the GUI responsive during large dataset processing.
- **Pre-requisites**: A directory containing raw DRR projections (`*_Proj_*.tif` or `*.png`).
- **Expected Next Step**: `.bin` files are used for subsequent tracking validation and viewer display in the orientation-matching monitors.
