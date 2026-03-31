# downsampling Module

This module reduces the resolution of 3D CT volumes to accelerate registration and preview generation.

## Architecture: Dispatch Pattern
- **`run.py`**: Dispatches to the appropriate implementation based on `dataset_type`.
- **`implementations/clinical.py`**: Standard 2x downsampling for clinical volumes.
- **`implementations/spare.py`**: Downsampling optimized for SPARE MetaImage volumes.

## Process
1. **Resampling**: Uses SimpleITK to rescale the volume (typically by a factor of 2 in each dimension).
2. **Metadata Preservation**: Ensures the origin and direction of the volume are preserved so registration matches across resolutions.
3. **Naming**: Output files are prefixed with `sub_` (e.g., `sub_CT_01.mha`).

## Usage
```python
from modules.downsampling.run import run
run(train_dir, dataset_type="spare")
```
