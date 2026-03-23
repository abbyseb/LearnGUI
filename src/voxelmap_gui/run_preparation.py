# run_preparation.py - COMPLETE SESSION-BASED VERSION
"""
Run preparation, validation, and directory management for Voxelmap pipeline.
IEC 62304-compliant validation with defense-in-depth approach.

Key Features:
- Session-based rerun support (checks train/ + intermediates/)
- Prerequisite-based availability checking
- Safe purging (train/ only, never intermediates/)
- Friendly user messages with file locations
"""
from __future__ import annotations
from pathlib import Path
from typing import List, Tuple, Dict
import re


# ============================================================================
# PATH/ID HELPERS
# ============================================================================

def patient_folder_from_rt(rt: str) -> str:
    """
    Extract patient folder name from rt_ct_pres.
    
    Example: '/VALKIM/RNSH/PlanningFiles/Patient01/' -> 'Patient01'
    """
    rt = (rt or "").strip("/")
    parts = rt.split("/")
    return parts[-1] if parts else ""


def centre_from_rt(rt: str) -> str:
    """
    Extract centre from rt_ct_pres.
    
    Example: '/TRIAL/CENTRE/...' -> 'CENTRE'
    """
    parts = [p for p in (rt or "").strip("/").split("/") if p]
    return parts[1] if len(parts) >= 2 else "?"


def patient_id_from_rt(rt: str, trial: str) -> str:
    """
    Extract patient ID from rt_ct_pres.
    
    Example: '/.../Patient01/' with trial='VALKIM' -> 'VALKIM_01'
    """
    tail = patient_folder_from_rt(rt)
    m = re.search(r'Patient(\d+)', tail, re.IGNORECASE)
    if m:
        return f"{trial}_{m.group(1)}"
    return f"{trial}/{tail}"


def train_src_from_rt(rt: str, base_dir: str) -> Path:
    """
    Map rt_ct_pres to MAGIKmodel/FilesForModelling source directory (read-only).
    base_dir is PRJ-RPL: we only read from it; never write or edit under it.

    Example: '/VALKIM/RNSH/PlanningFiles/Patient01/'
         -> 'Z:/PRJ-RPL/.../VALKIM/RNSH/MAGIKmodel/FilesForModelling'
    """
    parts = [p for p in (rt or "").strip("/").split("/") if p]
    if len(parts) >= 3:
        parts[2] = "MAGIKmodel"
    parts.append("FilesForModelling")
    return Path(base_dir).joinpath(*parts)


def selection_line(trial: str, centre_label: str, ids: list[str]) -> str:
    """Format selection summary for logging."""
    pats = ", ".join(ids) if ids else "—"
    return f"Patients selected: ['{trial}'/'{centre_label}'/'{pats}']"


# ============================================================================
# DICOM COUNTING
# ============================================================================

def count_dicoms_recursive(path: Path) -> int:
    """Count DICOM files in a source tree (for copy progress)."""
    import os
    total = 0
    if not path.exists():
        return 0
    for root, _, files in os.walk(path):
        for n in files:
            p = Path(root) / n
            suf = p.suffix.lower()
            if suf in (".dcm", ".dicom", ".ima"):
                total += 1
            elif suf == "":
                try:
                    with open(p, "rb") as fh:
                        fh.seek(128)
                        if fh.read(4) == b"DICM":
                            total += 1
                except Exception:
                    pass
    return total


def snapshot_dicoms(root: Path) -> tuple[int, int, dict[int, int]]:
    """
    Fast snapshot: (file_count, total_bytes, multiset_by_size).
    Used for prerequisite checking.
    """
    cnt, total, sizes = 0, 0, {}
    if not root.exists():
        return 0, 0, {}
    
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        ext = p.suffix.lower()
        if ext not in {".dcm", ".dicom", ".ima"}:
            continue
        try:
            sz = p.stat().st_size
        except Exception:
            continue
        cnt += 1
        total += sz
        sizes[sz] = sizes.get(sz, 0) + 1
    return cnt, total, sizes


# ============================================================================
# STEP DEFINITIONS - FIX: ALL KEYS ARE NOW UPPERCASE
# ============================================================================

STEP_OUTPUT_PATTERNS = {
    "COPY": { # ADD THIS BLOCK
        "outputs": [
            "*.dcm",
            "*.dicom",
            "*.ima",
        ],
        "description": "Copied DICOM files (recursive)",
    },
    "DICOM2MHA": {
        "outputs": [
            "CT_*.mha",
            "Body.mha",
            "GTV_*.mha", 
            "Lungs_*.mha",
            "SeriesInfo.txt",
        ],
        "description": "CT volumes and structure masks",
    },
    "DOWNSAMPLE": {
        "outputs": [
            "sub_CT_*.mha",
            "sub_Lungs_*.mha",
        ],
        "description": "Downsampled volumes",
    },
    "DRR": {
        "outputs": [
            "*_Proj_*.hnc",
        ],
        "description": "DRR projection images (HNC format)",
    },
    "COMPRESS": {
        "outputs": [
            "*_Proj_*.png",
            "*_Proj_*.tif",
            "*_Proj_*.tiff",
        ],
        "description": "Compressed DRR images",
    },
    "DVF2D": {
        "outputs": [
            "*2DDVF*.mat",
            "*2DDVF*.bin",
        ],
        "description": "2D displacement vector fields",
    },
    "DVF3D_LOW": { # ADDED
        "outputs": ["DVF_sub_*.mha"],
        "description": "Downsampled 3D DVFs",
    },
    "DVF3D_FULL": { # ADDED
        "outputs": ["DVF_*.mha", "!DVF_sub_*.mha"], # Exclude sub
        "description": "Full-resolution 3D DVFs",
    },
    "KV_PREPROCESS": { # ADDED
        "outputs": ["test/**/*.bin", "test/**/*.tif", "test/**/*.tiff"],
        "description": "Copied test files and kV binaries",
    },
}


STEP_PREREQUISITES = {
    "COPY": {
        "required_patterns": [],
        "description": "Patient selection (no file prerequisites)",
    },
    "DICOM2MHA": {
        "required_patterns": [],  # DICOMs checked separately via snapshot_dicoms
        "description": "DICOM files",
    },
    "DOWNSAMPLE": {
        "required_patterns": ["CT_*.mha", "Lungs_*.mha"],
        "description": "CT volumes and lung masks from DICOM→MHA",
    },
    "DRR": {
        "required_patterns": ["sub_CT_*.mha"],
        "description": "Downsampled CT volumes",
    },
    "COMPRESS": {
        "required_patterns": ["*_Proj_*.hnc"],
        "description": "DRR projections (HNC format)",
    },
    "DVF2D": {
        "required_patterns": ["*_Proj_*.png", "*_Proj_*.tif", "*_Proj_*.tiff"], # MODIFIED
        "description": "Compressed DRR images",
    },
    "DVF3D_LOW": {
        "required_patterns": ["sub_CT_*.mha"],
        "description": "Downsampled CT volumes",
    },
    "DVF3D_FULL": {
        "required_patterns": ["CT_*.mha"],
        "description": "Full-resolution CT volumes",
    },
    "KV_PREPROCESS": { # ADDED
        "required_patterns": [], # Requires rt_ct_pres, not files
        "description": "Patient selection (no file prerequisites)",
    },
}


STEP_DOWNSTREAM_DEPENDENCIES = {
    "COPY": ["DICOM2MHA", "KV_PREPROCESS"], # MODIFIED
    "DICOM2MHA": ["DOWNSAMPLE", "DVF3D_FULL"],
    "DOWNSAMPLE": ["DRR", "DVF3D_LOW"],
    "DRR": ["COMPRESS"],
    "COMPRESS": ["DVF2D"],
    "DVF2D": [],
    "DVF3D_LOW": [],
    "DVF3D_FULL": [],
    "KV_PREPROCESS": [], # ADDED
}


# ============================================================================
# STEP METADATA ACCESSORS
# ============================================================================

def get_step_output_info(step: str) -> Dict[str, any]:
    """Get output patterns and description for a step."""
    return STEP_OUTPUT_PATTERNS.get(step.upper(), {
        "outputs": [], 
        "description": "Unknown step"
    })


def get_step_prerequisite_info(step: str) -> Dict[str, any]:
    """Get prerequisite patterns and description for a step."""
    return STEP_PREREQUISITES.get(step.upper(), {
        "required_patterns": [], 
        "description": "Unknown requirements"
    })


def get_downstream_steps(step: str) -> List[str]:
    """Get list of steps that depend on this step's outputs."""
    return STEP_DOWNSTREAM_DEPENDENCIES.get(step.upper(), [])


# ============================================================================
# PREREQUISITE CHECKING (TRAIN/ + INTERMEDIATES/)
# ============================================================================

def check_step_prerequisites_exist(
    train_dir: Path, 
    patient_root: Path,
    step: str
) -> Tuple[bool, str]:
    """
    Check if prerequisites exist for a step (in train/ OR intermediates/).
    This determines if a step CAN be run, not if it WAS run.
    
    Used for dynamic rerun availability checking.
    
    Args:
        train_dir: Path to train/ directory
        patient_root: Path to patient root directory (contains train/ and intermediates/)
        step: Step identifier (e.g., "DICOM2MHA", "DOWNSAMPLE")
    
    Returns:
        (can_run, description) where description explains what was found or missing
    """
    intermediates_dir = patient_root / "intermediates"
    
    step_upper = step.upper()
    prereq_info = get_step_prerequisite_info(step_upper)

    # Special case: COPY and KV_PREPROCESS have no file prerequisites
    if step_upper == "COPY" or step_upper == "KV_PREPROCESS": # MODIFIED
        return True, "Patient selection is the only prerequisite"
    
    # Special case: DICOM→MHA needs DICOMs
    if step_upper == "DICOM2MHA":
        train_dcm, _, _ = snapshot_dicoms(train_dir)
        archive_dcm = 0
        if intermediates_dir.exists():
            archive_dcm, _, _ = snapshot_dicoms(intermediates_dir)
        
        total = train_dcm + archive_dcm
        if total == 0:
            return False, "No DICOM files available (source files deleted or not yet copied)"
        
        # Success - provide details
        parts = []
        if train_dcm > 0:
            parts.append(f"{train_dcm} in train/")
        if archive_dcm > 0:
            parts.append(f"{archive_dcm} in intermediates/")
        return True, f"{total} DICOM files available ({', '.join(parts)})"
    
    # Other steps: check file patterns
    required = prereq_info["required_patterns"]
    if not required:
        return True, "No prerequisites required"
    
    missing = []
    found_summary = []
    
    for pattern in required:
        train_count = len(list(train_dir.glob(pattern)))
        archive_count = 0
        if intermediates_dir.exists():
            archive_count = len(list(intermediates_dir.glob(pattern)))
        
        total = train_count + archive_count
        
        if total == 0:
            missing.append(pattern)
        else:
            # Build readable summary
            locations = []
            if train_count > 0:
                locations.append(f"{train_count} in train/")
            if archive_count > 0:
                locations.append(f"{archive_count} in intermediates/")
            
            # Use a friendly name for the pattern
            friendly_name = _pattern_to_friendly_name(pattern)
            found_summary.append(f"{friendly_name}: {', '.join(locations)}")
    
    if missing:
        missing_friendly = [_pattern_to_friendly_name(p) for p in missing[:2]]
        return False, f"Missing prerequisites: {', '.join(missing_friendly)}. Check train/ and intermediates/ folders."
    
    return True, " & ".join(found_summary)


def _pattern_to_friendly_name(pattern: str) -> str:
    """Convert glob pattern to user-friendly name."""
    mapping = {
        "CT_*.mha": "CT volumes",
        "sub_CT_*.mha": "Downsampled CT volumes",
        # ... (existing) ...
        "*_Proj_*.tif": "Compressed DRRs (TIF)",
        "*_Proj_*.tiff": "Compressed DRRs (TIFF)",
        "DVF_sub_*.mha": "Downsampled DVFs", # ADDED
        "DVF_*.mha": "Full-res DVFs", # ADDED
        "test/**/*.bin": "kV binaries", # ADDED
        "test/**/*.tif": "kV TIFFs", # ADDED
        "test/**/*.tiff": "kV TIFFs", # ADDED
    }
    return mapping.get(pattern, pattern)


# ============================================================================
# RERUN VALIDATION (CHECKS BOTH LOCATIONS)
# ============================================================================

def validate_rerun_prerequisites(
    pairs: list[tuple[Path, Path]],
    patient_root: Path,
    step: str,
    audit_log
) -> Tuple[bool, str]:
    """
    Validate prerequisites exist for rerun (checks train/ OR intermediates/).
    
    This is called by controller before purging outputs.
    
    Args:
        pairs: List of (src, train_dir) tuples (src ignored for rerun)
        patient_root: Patient root directory (contains train/ and intermediates/)
        step: Step to validate (e.g., "DICOM2MHA", "DOWNSAMPLE")
        audit_log: Audit logger for detailed logging
    
    Returns:
        (success, error_message) - error_message is empty string on success
    """
    step_upper = step.upper()
    audit_log.add(f"--- RERUN VALIDATION: {step_upper} ---", level="HEADER")
    
    for idx, (src, dst_train) in enumerate(pairs):
        patient = patient_root.name
        audit_log.add(f"Validating patient {idx+1}/{len(pairs)}: {patient}")
        
        # Check prerequisites (both locations)
        can_run, details = check_step_prerequisites_exist(dst_train, patient_root, step_upper)
        
        if not can_run:
            audit_log.add(f"  ✗ {details}", level="VALIDATE")
            audit_log.add("--- RERUN VALIDATION: FAILED ---", level="HEADER")
            return False, details
        
        audit_log.add(f"  ✓ {details}", level="VALIDATE")
    
    audit_log.add("--- RERUN VALIDATION: PASSED ---", level="HEADER")
    return True, ""


# ============================================================================
# PURGE OUTPUTS (TRAIN/ ONLY)
# ============================================================================

def purge_step_outputs_from_train(
    train_dir: Path,
    step: str,
    work_root: Path,
    audit_log
) -> int:
    """
    Purge outputs of a specific step from train/ only.
    Does NOT touch intermediates/ (preserves for future reruns).
    
    Args:
        train_dir: Path to train/ directory
        step: Step identifier (e.g., "DICOM2MHA")
        work_root: Workspace root (for safety check)
        audit_log: Audit logger
    
    Returns:
        Number of files deleted
    """
    try:
        # Safety check: must be in work_root
        wr = work_root.resolve(strict=False)
        dt = Path(train_dir).resolve(strict=False)
        if wr not in dt.parents and wr != dt:
            audit_log.add("Safety check failed - train_dir not in work_root", level="ERROR")
            return 0
        
        step_upper = step.upper()
        output_info = get_step_output_info(step_upper)
        patterns = output_info["outputs"]
        
        if not patterns:
            audit_log.add(f"No output patterns defined for {step_upper}", level="WARN")
            return 0
        
        audit_log.add(f"Purging outputs for {step_upper} from train/:")
        deleted = 0
        
        for pattern in patterns:
            # Skip SeriesInfo.txt unless DICOM2MHA
            if pattern == "SeriesInfo.txt" and step_upper != "DICOM2MHA":
                continue

            # Handle exclusion patterns (like !DVF_sub_*.mha)
            exclude_pattern = None
            if pattern.startswith("!"):
                exclude_pattern = pattern[1:]
                continue # This is just a modifier for other patterns
            
            # MODIFIED: Use rglob for COPY and KV_PREPROCESS steps
            if step_upper == "COPY" or step_upper == "KV_PREPROCESS":
                glob_iter = train_dir.rglob(pattern)
            else:
                glob_iter = train_dir.glob(pattern)
            
            for file_path in glob_iter:
                if not file_path.is_file():
                    continue

                # Apply exclusion
                if exclude_pattern and file_path.match(exclude_pattern):
                    continue
                
                # MODIFIED: Safety check for rglob
                if step_upper not in ["COPY", "KV_PREPROCESS"]:
                    try:
                        rel = file_path.relative_to(train_dir)
                        if len(rel.parts) > 1:  # In a subdirectory
                            continue
                    except ValueError:
                        continue
                
                try:
                    file_path.unlink()
                    deleted += 1
                except Exception as e:
                    audit_log.add(f"Failed to delete {file_path.name}: {e}", level="ERROR")
        
        return deleted
        
    except Exception as e:
        audit_log.add(f"Error during purge: {e}", level="ERROR")
        return 0


# ============================================================================
# TRAIN PAIR BUILDING
# ============================================================================

def build_train_pairs_for_run(rt_list: List[str], run_folder: Path, base_dir: str) -> list[tuple[Path, Path]]:
    """
    Build pairs for a run folder, pointing to the correct MATLAB-created
    patient subfolder (e.g., .../Run_.../Patient01/train).
    base_dir (PRJ-RPL) is read-only: src_path is read from, train_dir is under work_root.

    Args:
        rt_list: List of rt_ct_pres paths
        run_folder: Run folder path (e.g., .../VALKIM_001_Run_20251015_143045)
        base_dir: Base directory for source data (read-only; never write to it)

    Returns:
        List of (src_path, train_dir) tuples
    """
    pairs = []
    # This assumes a single patient per run, which is consistent with the UI flow.
    # The watchers will monitor the destination path generated here.
    if rt_list:
        patient_folder = patient_folder_from_rt(rt_list[0])
        train_dir = run_folder / patient_folder / "train"
        for rt in rt_list:
            src = train_src_from_rt(rt, base_dir)
            pairs.append((src, train_dir))
    
    return pairs


# ============================================================================
# MHA SCANNING (FOR VIEWER)
# ============================================================================

def scan_mha_files(dest_trains: List[Path]) -> list[Path]:
    """
    Return sorted list of CT_*.mha files from the first train/ directory.
    Used by CT viewer to populate phase selector.
    """
    if not dest_trains:
        return []
    train = dest_trains[0]
    return sorted(train.glob("CT_*.mha"))


def scan_overlay_sources(dest_trains: List[Path]) -> dict[str, str]:
    """
    Return {name: abs_path} for available structure overlays in the first train/.
    Used by CT viewer for overlay selection.
    """
    out = {}
    if not dest_trains:
        return out
    train = dest_trains[0]
    candidates = {
        "Body": "Body.mha",
        "GTV_Inh": "GTV_Inh.mha",
        "GTV_Exh": "GTV_Exh.mha",
        "Lungs_Inh": "Lungs_Inh.mha",
        "Lungs_Exh": "Lungs_Exh.mha",
    }
    for k, fn in candidates.items():
        p = (train / fn)
        try:
            if p.exists():
                out[k] = str(p)
        except Exception:
            pass
    return out


# ============================================================================
# SERIES INFO HELPER
# ============================================================================

def read_expected_ct_count_from_seriesinfo(train_dir: Path) -> int | None:
    """
    Read expected CT_*.mha count from SeriesInfo.txt.
    Used by MHA watcher to determine total count.
    
    Returns:
        Expected count, or None if SeriesInfo.txt doesn't exist or can't be parsed
    """
    series_info = train_dir / "SeriesInfo.txt"
    if not series_info.exists():
        return None
    try:
        count = 0
        with series_info.open("r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                if re.search(r"\bCT_\d{2}\s*:", line):
                    count += 1
        return count if count > 0 else None
    except Exception:
        return None