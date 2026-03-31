# drr_compression Module

This module compresses raw PNG projections into binary files for efficient pipeline processing.

## Architecture: Dispatch Pattern
- **`run.py`**: Dispatches to the appropriate implementation based on `dataset_type`.
- **`implementations/clinical.py`**: Standard PNG-to-BIN conversion (uint8).

## Process
1. **Serialization**: Combines PNG frames into a single contiguous binary block.
2. **Metadata Header**: Attaches necessary spatial and intensity metadata to the binary file.

## Usage
```python
from modules.drr_compression.run import run
run(train_dir, dataset_type="clinical")
```
