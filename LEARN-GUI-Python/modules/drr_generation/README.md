# drr_generation Module

This module generates Digitally Reconstructed Radiographs (DRRs) from 3D CT volumes, simulating a radiotherapy gantry rotation.

## Files
- **`generate.py`**: The main execution script using ITK-RTK. It handles the forward projection of a CT volume into a stack of 2D images.
- **`RTKGeometry_360.xml`**: Pre-defined RTK geometry file containing 120 projection points over a 360-degree rotation (3-degree steps).

## Methodology
The generation process uses the following clinical configurations:
1. **Joseph Forward Projection**: Utilizes the `rtk.JosephForwardProjectionImageFilter` for fast and accurate simulation of X-ray transmission.
2. **Circular Geometry**: By default, it uses a 360-degree orbit with:
    - **SID (Source-to-Isocenter Distance)**: 1000 mm
    - **SDD (Source-to-Detector Distance)**: 1500 mm
    - **Projections**: 120 frames (3-degree increments).
3. **Output Normalization**: Resulting projections are min-max normalized per CT volume and saved as **8-bit uint8 PNG** files (`CTnum_Proj_Index.png`).
4. **Orientation Parity**: Works in conjunction with the `dicom2mha` module's LSA orientation to ensure the vertical axis of the projection matches the superior-inferior axis of the patient.

## Pipeline Integration
- **Bigger Picture**: This module creates the "synthetic" X-rays used to match with real kV images during patient setup verification.
- **GUI Connection**: Triggered via the "DRR GENERATION" button in the GUI. The results are immediately visualizable in the orientation-matching monitors.
- **Pre-requisites**: A valid 3D CT volume (`.mha`).
- **Expected Next Step**: Raw PNG projections are processed by the **drr_compression** module into `.bin` format.
