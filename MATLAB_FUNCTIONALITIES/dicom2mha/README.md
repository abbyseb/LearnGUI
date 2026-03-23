# DICOM to MetaImage (MHA) Conversion

This component is responsible for the initial processing of patient data, converting clinical DICOM files into research-ready MetaImage (.mha) volumes.

## Purpose
- Extract 3D CT volumes from DICOM series.
- Generate binary structure masks (e.g., GTV, Lungs) from RT Structure Sets.
- Ensure correct spatial alignment and coordinate system consistency (LPS/DICOM standard).

## Key Scripts and Files
- **`DicomToMha_VALKIM.m`**: The main entry point for the VALKIM trial data. Orchestrates the conversion of CT and RTStruct files.
- **`ConvertDCMVolumesToMHA.m`**: Core logic for reading DICOM series and writing MHA volumes.
- **`DICOM2IEC.m`**: Handles coordinate system transformations from DICOM to the IEC 61217 standard.
- **`dicomDscrptToPhaseBin.m`**: Utility to parse DICOM series descriptions to extract respiratory phase information.
- **`MhaWrite.m`**: Low-level utility for writing MetaImage (.mha) files.
- **`mhaHeaderTemplate.mat`**: MATLAB data file containing the template header structure for MHA files.

## Outputs
- `CT_XX.mha`: The 3D CT volume for phase XX.
- `Lungs_XX.mha`: Binary mask for the lungs in phase XX.
- `GTV_XX.mha`: Binary mask for the Gross Tumor Volume in phase XX.
- `SeriesInfo.txt`: Metadata summary of the processed series.
