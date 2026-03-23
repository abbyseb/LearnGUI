# LEARN-GUI

A PyQt6 interface for preparing medical imaging data for AI-guided radiotherapy, orchestrating MATLAB pipelines for DICOM processing, DRR generation, and deformation vector field creation.

## Prerequisites

- **Python 3.11** (not 3.12+)
- **MATLAB**: On Windows use R2024a at `C:/Program Files/MATLAB/R2024a/`; on macOS use R2025b at `/Applications/MATLAB_R2025b.app/`
- **Poetry** (`pip install poetry`)
- **Data access**: On Windows, Z: drive to PRJ-LEARN and PRJ-RPL; on macOS, volumes mounted at `/Volumes/PRJ-LEARN` and `/Volumes/PRJ-RPL`

## Installation and Usage

### Option 1: Using the Setup Script (Windows Only - Recommended)
A PowerShell script is provided to automate creating the Python virtual environment and installing all necessary dependencies.

1. Open PowerShell and run the setup script:
   ```powershell
   .\setup_local.ps1
   ```
2. Once complete, run the application using the local environment:
   ```powershell
   .\venv\Scripts\python.exe -m voxelmap_gui
   ```

### Option 2: Using Poetry (Cross-Platform)
```bash
git clone https://github.sydney.edu.au/Image-X/LEARN-GUI.git
cd LEARN-GUI
poetry install
poetry run python -m voxelmap_gui
```

## Configuration

Paths are defined in `src/voxelmap_gui/backend.py` (platform-dependent); `mainwindow.py` uses `WORK_ROOT` from backend.

| Variable | Windows | macOS |
|----------|---------|-------|
| `MATLAB_BIN` | `C:/Program Files/MATLAB/R2024a/bin/matlab.exe` | `/Applications/MATLAB_R2025b.app/bin/matlab` |
| `MATLAB_CODE_DIR` | `Z:/PRJ-LEARN/LEARN/GUI/MATLAB_code` | Project `matlab_files/` |
| `MATLAB_EXTRA_DIR` | `Z:/PRJ-LEARN/LEARN/GUI/MATLAB` | Project `matlab_files/` |
| `BASE_DIR` | `Z:/PRJ-RPL/2RESEARCH/1_ClinicalData` | `/Volumes/PRJ-RPL/2RESEARCH/1_ClinicalData` |
| `WORK_ROOT` | `Z:\PRJ-LEARN\LEARN\GUI\Runs` | `/Volumes/PRJ-LEARN/LEARN/GUI/Runs` |

Edit `backend.py` to change paths for your environment.

# LearnGUI
