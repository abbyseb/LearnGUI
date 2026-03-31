# dvf_generation Module

This module generates 3D Displacement Vector Fields (DVFs) via deformable image registration (DIR).

## Architecture: Dispatch Pattern
- **`run.py`**: Dispatches to the appropriate implementation based on `dataset_type`.
- **`implementations/clinical.py`**: Standard 3D registration using Elastix-BSpline.

## Core Features
1. **Deformable Registration**: Registers a moving volume to a fixed reference (typically phase 06).
2. **Component Separation**: Output contains the full displacement vector field.
3. **SPARE Support**: While using the standard BSpline engine, the naming and re-orientation logic in the GUI is adjusted for SPARE axes.

## Usage
```python
from modules.dvf_generation.run import run
run(fixed_path, moving_path, param_path, output_path, dataset_type="clinical")
```
