# run_folder_manager.py
"""
Manages run folder creation and organization for medical device traceability.
Each pipeline run creates a unique timestamped folder with dedicated logs.
"""
from __future__ import annotations
from pathlib import Path
from datetime import datetime
from typing import Optional
import re


def generate_run_folder_name(patient_id: str) -> str:
    """
    Generate unique run folder name.
    Format: {patient_id}_Run_{YYYYMMDD_HHMMSS}
    Example: VALKIM_001_Run_20250115_143045
    """
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return f"{patient_id}_Run_{timestamp}"


def create_run_structure(work_root: Path, patient_id: str) -> tuple[Path, Path]:
    """
    Create folder structure for a new run.
    
    Returns:
        (run_folder, log_file)
        
    Structure:
        work_root/
            VALKIM_001_Run_20250115_143045/  <- run_folder
            logs/
                VALKIM_001_Run_20250115_143045/
                    run_20250115_143045.log   <- log_file
    """
    run_name = generate_run_folder_name(patient_id)
    run_folder = work_root / run_name
    run_folder.mkdir(parents=True, exist_ok=True)
    
    # Create external, per-run log directory
    logs_dir = work_root / "logs" / run_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    
    # Create log file
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_file = logs_dir / f"run_{timestamp}.log"
    log_file.touch()
    
    return run_folder, log_file


def list_patient_runs(work_root: Path, patient_id: str) -> list[Path]:
    """
    List all existing run folders for a patient.
    Returns sorted by timestamp (newest first).
    """
    pattern = f"{patient_id}_Run_*"
    runs = list(work_root.glob(pattern))
    
    # Sort by timestamp in folder name (newest first)
    runs.sort(reverse=True)
    return runs


def get_run_log_file(run_folder: Path) -> Optional[Path]:
    """Get the log file for a run folder from the external logs directory."""
    work_root = run_folder.parent
    logs_dir = work_root / "logs" / run_folder.name
    if not logs_dir.exists():
        return None
    
    # Find most recent log (in case multiple)
    logs = sorted(logs_dir.glob("run_*.log"), reverse=True)
    return logs[0] if logs else None


def extract_patient_id(run_folder_name: str) -> str:
    """
    Extract patient ID from run folder name.
    VALKIM_001_Run_20250115_143045 -> VALKIM_001
    """
    match = re.match(r"^(.+?)_Run_\d{8}_\d{6}$", run_folder_name)
    if match:
        return match.group(1)
    return run_folder_name


def get_train_dir(run_folder: Path) -> Path:
    """
    Get train/ directory for a run folder, looking for a patient subfolder
    created by MATLAB (e.g., .../Run_.../Patient01/train).
    """
    # Find the patient subfolder created by MATLAB
    patient_subfolder = None
    for item in run_folder.iterdir():
        if item.is_dir() and item.name not in ["logs", "_matlab_prefs", "test"]:
            patient_subfolder = item
            break  # Assume only one such folder per run
    
    if patient_subfolder:
        return patient_subfolder / "train"
        
    # Fallback for safety, though the above path is expected
    return run_folder / "train"


def get_test_dir(run_folder: Path) -> Path:
    """Get test/ directory for a run folder."""
    return run_folder / "test"


def validate_run_folder(run_folder: Path) -> bool:
    """Validate that a folder is a proper run folder structure."""
    if not run_folder.exists() or not run_folder.is_dir():
        return False
    # A valid run folder must have a corresponding external log directory
    log_file = get_run_log_file(run_folder)
    return log_file is not None and log_file.exists()