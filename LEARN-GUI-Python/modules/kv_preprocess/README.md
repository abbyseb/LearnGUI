# kv_preprocess Module

This module handles the organization and preprocessing of clinical kV (X-ray) images for patient tracking validation.

## Files
- **`process.py`**: The main execution script. It manages the copying of "Treatment files" (test data) and the conversion of raw X-ray images into binary format.

## Methodology
The module implements the following clinical data management steps:
1. **Data Organization**: Automatically identifies the correct "1_ClinicalData" or "PRJ-RPL" source directory and copies the relevant "Treatment files" into the local patient `test/` directory.
2. **kV Image Conversion**: Scans each treatment fraction (e.g., `Fx01`, `Fx02`) for `kV` folders and processes all image formats (`.tif`, `.tiff`, `.png`).
3. **Binary Formatting**: Each kV image is converted to a grayscale **uint8** array (via PIL) and then saved as a raw **float32** binary file (`.bin`).
4. **Registration Prep**: Ensures that the `source.mha` (reference CT) is available in the test folders for the subsequent tracking registration process.

## Pipeline Integration
- **Bigger Picture**: This is the final preparation step for validaiton. It organizes real patient data (kV images) into the structure required by the clinical tracking algorithm.
- **GUI Connection**: Triggered via the "KV PREPROCESS" step in the clinical workflow. It ensures that the "test" data is ready for the tracking monitors.
- **Pre-requisites**: Valid clinical data structure (with `1_ClinicalData` or `PRJ-RPL` markers) and raw kV images.
- **Expected Next Step**: The preprocessed kV images and reference CT are utilized by the **Tracking Validation** engine to estimate real-time anatomy motion.
