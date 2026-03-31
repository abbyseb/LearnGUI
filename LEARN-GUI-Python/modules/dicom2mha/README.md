# dicom2mha Module

This module handles the conversion of planning CT data from its raw source format (DICOM or MetaImage) into a standardized nomenclature used by the pipeline.

## Architecture: Dispatch Pattern
The module follows a **dispatch pattern** to handle different dataset types independently:
- **`run.py`**: The entry point. It imports the correct implementation based on the `dataset_type` parameter (e.g., `clinical` or `spare`).
- **`implementations/clinical.py`**: Handles traditional DICOM-to-MHA conversion.
- **`implementations/spare.py`**: Handles SPARE MetaImage volume renaming and nomenclature normalization.

## Dataset Types

### 1. Clinical (DICOM)
- **Source**: Nested DICOM folders.
- **Process**: Uses SimpleITK's `ImageSeriesReader` to group slices into 3D volumes.
- **Output**: MetaImage (`.mha`) volumes named by series number (e.g., `CT_01.mha`).

### 2. SPARE (MHA/MHD)
- **Source**: Pre-converted `.mha` volumes.
- **Process**: Renames volumes to follow the `CT_XX.mha` pattern and generates a `SeriesInfo.txt` for pipeline tracking.
- **Output**: Standardized `.mha` volumes (e.g., `CT_06.mha` for the fixed reference).

## Usage
```python
from modules.dicom2mha.run import run
run(source_path, target_path, dataset_type="spare")
```
