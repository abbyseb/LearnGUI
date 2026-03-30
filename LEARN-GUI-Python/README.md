# Learn-GUI-Python

Native Python version of the LEARN-GUI application. **No MATLAB required.**

All pipeline steps (DICOM→MHA, downsampling, DRR generation, compression, DVF 2D/3D, kV preprocess) are implemented in pure Python using SimpleITK, ITK-RTK, and ITK-Elastix.

## Quick Start

```bash
pip install -r requirements.txt
python main.py
```

## Data Root

Set your clinical data path (base: `/PRJ-RPL/2RESEARCH/1_ClinicalData`):

- Use **Browse...** in the Case Selection panel, or
- `export LEARN_DATA_ROOT="/Volumes/research-data/PRJ-RPL/2RESEARCH/1_ClinicalData"`

**SMB share:** `smb://shared.sydney.edu.au/research-data/PRJ-RPL/2RESEARCH/1_ClinicalData`  
On Mac, mount via Finder (Go → Connect to Server) or:
```bash
open "smb://shared.sydney.edu.au/research-data"
```
Then the path is `/Volumes/research-data/PRJ-RPL/2RESEARCH/1_ClinicalData`.

## Pipeline Steps

1. **Copy** – Copy DICOMs from source to run folder
2. **DICOM → MHA** – Convert DICOM CT to MHA volumes
3. **Downsample** – Downsample volumes to 128³
4. **DRR** – Generate DRR projections (ITK-RTK)
5. **Compress** – Compress projections to binary format
6. **DVF 2D** – Generate 2D displacement vector fields
7. **DVF 3D (low/full)** – Generate 3D DVFs (ITK-Elastix)
8. **kV Preprocess** – Copy test data and process kV images

## Project Structure

- `main.py` – Entry point
- `gui/` – PyQt6 interface
- `modules/` – Pure Python pipeline implementations
