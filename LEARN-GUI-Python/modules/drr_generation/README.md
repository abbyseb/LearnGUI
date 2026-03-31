# drr_generation Module

This module generates Digitally Reconstructed Radiographs (DRRs) from 3D CT volumes.

## Architecture: Dispatch Pattern
- **`run.py`**: Dispatches to the appropriate implementation based on `dataset_type`.
- **`implementations/clinical.py`**: Traditional forward projection.
- **`implementations/spare.py`**: Specialized DRR handling for the SPARE dataset.

## Output
- **Format**: PNG images.
- **Naming**: `CTnum_Proj_Index.png` (e.g., `06_Proj_001.png`).
- **Nomenclature**: For SPARE, result files are normalized to match the expected pipeline names.

## Usage
```python
from modules.drr_generation.run import run
run(mha_path, output_dir, geometry_file, dataset_type="clinical")
```
