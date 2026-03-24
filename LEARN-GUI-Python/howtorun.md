# How to Run LEARN-GUI-Python

This project is a standalone Python version of the LEARN-GUI application. It does NOT require MATLAB.

## 1. Prerequisites

- **Python 3.9+** is recommended.
- Ensure you have a C++ compiler or the appropriate runtime for `itk-rtk` and `itk-elastix` (usually handled by pip on Windows).

## 2. Install Requirements

You can install all necessary dependencies using the following command:

```bash
pip install PyQt6 SimpleITK itk itk-rtk itk-elastix numpy requests matplotlib scipy pydicom
```


Alternatively, if a `requirements.txt` is provided:

```bash
pip install -r requirements.txt
```

> [!IMPORTANT]
> If you see a "ModuleNotFoundError" or if the **DICOM Preview** (the cross-sectional views) is not showing during the COPY step, ensure you have installed all requirements using the command above. The preview relies on `pydicom` and `numpy`.


### 3. Usage
1.  **Launch the GUI**: Run `python main.py`.
2.  **Select Data Root**: If your data is on a network drive (e.g., `Z:/`), click the **Browse...** button in the top-left "Case Selection" panel and select the drive/folder.
3.  **Choose Patient**: Select a trial and patient ID from the list.
4.  **Run Pipeline**: Tick the desired steps and click **Run Pipeline**.

## 4. Project Structure

- **`main.py`**: The entry point.
- **`gui/`**: Refactored PyQt6 interface and logic.
- **`modules/`**: Native Python replacements for MATLAB functionalities.
- **`data/`**: Place your patient DICOM folders here for local processing.
- **`Runs/`**: Output directory for pipeline results.

## 5. Troubleshooting

- **Import Errors**: Ensure you are running `main.py` from the project root so that the `sys.path` addition works correctly.
- **itk-rtk/itk-elastix**: If these fail to install via pip, ensure your pip and setuptools are up to date: `pip install --upgrade pip setuptools`.
- **Display Warnings**: SimpleITK warnings are suppressed by default in `gui/app.py`.
