# kv_preprocess Module

This module handles the extraction and formatting of geometric metadata and labels for the registration pipeline.

## Architecture: Dispatch Pattern
- **`run.py`**: Dispatches to the appropriate implementation based on `dataset_type`.
- **`implementations/clinical.py`**: Extracts parameters from clinical metadata.
- **`implementations/spare.py`**: Extracts ground truth information and geometry from SPARE dataset files.

## Responsibilities
- **Geometry Extraction**: Parses SID/SDD/Orientation from dataset files.
- **Label Formatting**: Standardizes ground truth labels for use in training or validation.

## Usage
```python
from modules.kv_preprocess.run import run
run(rt_list, base_dir, train_dir, dataset_type="spare")
```
