# dicom2mha Module

This module handles the conversion of DICOM CT slice series into 3D MetaImage (MHA) volumes, following clinical radiotherapy standards.

## Files
- **`convert.py`**: Main entry point for the conversion process. It scans directories for CT DICOM files and orchestrates the conversion using either the gated-phase (VALKIM) or single-series path.
- **`valkim.py`**: A specialized path for handling gated 4D CT phases. It sorts slices into temporal bins (e.g., 0%, 10%, ... 90%) and assembles them into coherent 3D volumes.
- **`dicom2iec.py`**: Implements the conversion from DICOM coordinate space to the IEC-style voxel arrays used by the GUI. Handles intensity rescaling (Hounsfield Units) and water attenuation.
- **`mha_write.py`**: Utility to write uncompressed 32-bit float MetaImage files with specific anatomical headers (RAI orientation, offset, and spacing).
- **`phase_bin.py`**: Utility to extract the gating phase index (1-10) from DICOM series descriptions (e.g., "Gated 50%" -> Bin 6).
- **`dicom2iec.py`**: Contains core coordinate transformation logic.

## Methodology
The module performs several critical steps to ensure clinical parity:
1. **LSA Orientation**: DICOM volumes are re-sampled/permuted into **Left-Superior-Anterior (LSA)** orientation. This ensures the Superior-Inferior axis aligns with the treatment machine's rotation axis (Y), as required by RTK.
2. **HU Rescaling**: Applies `RescaleSlope` and `RescaleIntercept` to map raw pixel values to Hounsfield Units.
3. **Phase Sorting**: Uses SeriesInstanceUID and Gating percentages to group slices into temporal phases for 4D CT support.
4. **Water Attenuation**: Optionally scales HU values for specific clinical registration requirements.

## Pipeline Integration
- **Bigger Picture**: This is the very first step in the "Data Ingest" pipeline. It transforms raw scanner data into standardized volumes used by all subsequent tracking and registration modules.
- **GUI Connection**: Triggered when the user selects a DICOM directory and clicks "Load Data" or "DICOM 2 MHA". Progress is reported via phase callbacks.
- **Pre-requisites**: A directory containing valid CT DICOM slices (with `DICM` preamble).
- **Expected Next Step**: Standard `CT_XX.mha` volumes are created in the patient's `train/` or `test/` directory. The next step is typically **DOWNSAMPLLING** for low-res registration.
