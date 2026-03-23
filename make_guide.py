import json
import os

notebook_content = {
 "cells": [
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "# Voxelmap GUI – User Guide\n",
    "\n",
    "**Version:** 1.0.0  \n",
    "**Target:** Clinical Data Preparation for Radiotherapy \n",
    "\n",
    "## 1. Introduction\n",
    "This application is a interface designed to bridge Python (GUI, Visualisation) and MATLAB (Heavy Computation).\n",
    "\n",
    "It allows users to:\n",
    "1.  Select patients from the Clinical Drive (Windows: `Z:`; macOS: `/Volumes/PRJ-RPL/...`).\n",
    "2.  Orchestrate the radiotherapy processing pipeline.\n",
    "3.  Visualise results and outputs.\n",
    "4.  Audit every step for traceability in a logfile."
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "## 2. Prerequisites\n",
    "\n",
    "Before running this software, ensure the following are installed:\n",
    "\n",
    "* **Python 3.11**: Required; newer versions may not be compatible with all MATLAB releases.\n",
    "* **MATLAB**: Windows R2024a at `C:/Program Files/MATLAB/R2024a/`; macOS R2025b at `/Applications/MATLAB_R2025b.app/`. Paths are set in `backend.py`.\n",
    "* **Poetry**: The package manager ensures the exact same library versions.\n",
    "* **Network Access**: Access to clinical data (Windows: `Z:/PRJ-RPL/2RESEARCH/1_ClinicalData`; macOS: `/Volumes/PRJ-RPL/2RESEARCH/1_ClinicalData`). Check `backend.py` for your platform."
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "## 3. Installation & Setup\n",
    "\n",
    "Run the following commands in your terminal:"
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": [
    "# 1. Install dependencies defined in pyproject.toml\n",
    "!poetry install\n",
    "\n",
    "# 2. Verify MATLAB engine is accessible (paths in backend.py; Windows vs macOS)\n",
    "import os, sys\n",
    "sys.path.insert(0, 'src')\n",
    "from voxelmap_gui.backend import MATLAB_BIN\n",
    "if os.path.exists(MATLAB_BIN):\n",
    "    print(\"MATLAB found.\")\n",
    "else:\n",
    "    print(\"MATLAB missing - Check backend.py for your platform\")"
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "## 4. How to Use the Interface\n",
    "\n",
    "### A. Launching the App\n",
    "Run the cell below to launch the GUI. The application window will open separately."
   ]
  },
  {
   "cell_type": "code",
   "execution_count": None,
   "metadata": {},
   "outputs": [],
   "source": [
    "!poetry run voxelmap"
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "### B. The Workflow\n",
    "\n",
    "1.  **Case Selection (Top Left Panel):**\n",
    "    * The app scans the clinical drive for available trials and centres (Windows: `Z:`; macOS: `/Volumes/PRJ-RPL/...`).\n",
    "    * Select a trial/centre.\n",
    "    * Click a Patient ID in the list to load them. \n",
    "\n",
    "2.  **Pipeline Configuration (Bottom Left Tab):**\n",
    "    * Checkboxes: Select which steps to run\n",
    "    * Allowed Configurations: GUI highlights acceptable choice of steps.\n",
    "\n",
    "3.  **Data Visualisation (Right Panel):**\n",
    "    * CT Viewer: Axial/coronal/sagittal views with windowing and overlayed structs.\n",
    "    * DRR Viewer: Cine player for DRRs.\n",
    "    * DVF Viewer: Color heatmap overlays showing deformation."
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "## 5. Project Structure\n",
    "\n",
    "Here is the map of the `src` layout:\n",
    "\n",
    "```text\n",
    "src/voxelmap_gui/\n",
    "├── app.py              # Entry point. Handles global exception hooks\n",
    "├── controller.py       # Orchestrates UI - Backend logic\n",
    "├── backend.py          # Generates MATLAB command strings\n",
    "├── workers.py          # QThread wrapper for the MATLAB subprocess\n",
    "├── viewer_monitors.py  # Watches folders for new files to display\n",
    "└── ui/\n",
    "    ├── case_dock.py    # Patient selector logic\n",
    "    ├── panels.py       # Visualisation widgets (CT/DRR/DVF)\n",
    "    └── tabs.py         # Progress bars and step logic\n",
    "```"
   ]
  },
  {
   "cell_type": "markdown",
   "metadata": {},
   "source": [
    "## 6. Troubleshooting\n",
    "\n",
    "**Issue: The App freezes on startup.**\n",
    "* Cause: Network timeout connecting to the clinical drive or API (Windows: `Z:`; macOS: `/Volumes/PRJ-RPL` or PRJ-LEARN).\n",
    "* Fix: Ensure you are connected to the clinical network/VPN and have the correct API address.\n",
    "\n",
    "**Issue: MATLAB crashes or hangs.**\n",
    "* Cause: License error or memory issue.\n",
    "* Fix: Check the logs in the `logs/` folder. \n",
    "    * If the GUI is unresponsive, force-stop MATLAB (Windows: `taskkill /F /IM matlab.exe`; macOS: `pkill -x MATLAB` or Activity Monitor).\n",
    "\n",
    "**Issue: \"No module named voxelmap_gui\"**\n",
    "* Cause: Poetry environment not synced.\n",
    "* Fix: Run `poetry install`."
   ]
  }
 ],
 "metadata": {
  "kernelspec": {
   "display_name": "Python 3",
   "language": "python",
   "name": "python3"
  },
  "language_info": {
   "codemirror_mode": {
    "name": "ipython",
    "version": 3
   },
   "file_extension": ".py",
   "mimetype": "text/x-python",
   "name": "python",
   "nbconvert_exporter": "python",
   "pygments_lexer": "ipython3",
   "version": "3.11.0"
  }
 },
 "nbformat": 4,
 "nbformat_minor": 5
}

# Write the file
with open('VoxelMap_User_Guide.ipynb', 'w', encoding='utf-8') as f:
    json.dump(notebook_content, f, indent=1)

print("Successfully created 'VoxelMap_User_Guide.ipynb'")