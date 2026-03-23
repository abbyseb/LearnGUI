# setup_local.ps1 - VoxelMap Local Environment Setup
# This script recreates the Python virtual environment for Windows.

$VENV_DIR = "venv"
$PYTHON_VERSION = "3.11"

Write-Host "--- LEARN-GUI Local Setup ---" -ForegroundColor Cyan

# 1. Remove old venv if it exists
if (Test-Path $VENV_DIR) {
    Write-Host "Removing existing (possibly incompatible) virtual environment..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $VENV_DIR
}

# 2. Check for Python 3.11
Write-Host "Checking for Python $PYTHON_VERSION..." -ForegroundColor Cyan

# Check discovered user path first
$userPython = "C:\Users\aseb0376\AppData\Local\Programs\Python\Python311\python.exe"
if (Test-Path $userPython) {
    $pythonPath = $userPython
} else {
    $pythonPath = (Get-Command "python.exe" -ErrorAction SilentlyContinue).Source
    if ($null -eq $pythonPath) {
        # Try 'py' launcher
        $pythonPath = (py -3.11 -c "import sys; print(sys.executable)" 2>$null)
    }
}

if ($null -eq $pythonPath -or $pythonPath -eq "") {
    Write-Host "ERROR: Python $PYTHON_VERSION not found!" -ForegroundColor Red
    Write-Host "Please download and install Python $PYTHON_VERSION from: https://www.python.org/downloads/windows/" -ForegroundColor White
    Write-Host "Ensure 'Add Python to PATH' is checked during installation."
    exit 1
}

Write-Host "Using Python: $pythonPath" -ForegroundColor Green

# 3. Create new venv
Write-Host "Creating new virtual environment..." -ForegroundColor Cyan
& $pythonPath -m venv $VENV_DIR

if (!(Test-Path $VENV_DIR)) {
    Write-Host "Failed to create virtual environment." -ForegroundColor Red
    exit 1
}

# 4. Install dependencies
Write-Host "Installing dependencies..." -ForegroundColor Cyan
$pip = Join-Path $VENV_DIR "Scripts\pip.exe"

# Basic requirements from pyproject.toml
$deps = @(
    "PyQt6==6.9.0",
    "pyqtgraph==0.13.7",
    "SimpleITK==2.5.0",
    "pydicom==3.0.0",
    "imageio==2.37.0",
    "numpy>=2.0.0",
    "scipy>=1.14.0",
    "matplotlib>=3.9.0",
    "requests>=2.32.0",
    "pillow>=11.0.0"
)

& $pip install $deps

# Install the project itself in editable mode so 'voxelmap_gui' can be found
Write-Host "Installing LEARN-GUI package..." -ForegroundColor Cyan
& $pip install -e .

Write-Host "`n--- Setup Complete! ---" -ForegroundColor Green
Write-Host "To run the application, use:"
Write-Host ".\$VENV_DIR\Scripts\python.exe -m voxelmap_gui" -ForegroundColor Cyan
