# backend.py
import re
from pathlib import Path
import subprocess, platform, uuid, os

try:
    import requests
except Exception:
    requests = None
    
try:
    from scipy.io import loadmat
except ImportError:
    loadmat = None


# Platform-dependent path configuration (single source of truth)
#
# *** READ-ONLY: PRJ-RPL ***
# BASE_DIR points at PRJ-RPL (e.g. /Volumes/PRJ-RPL/... or Z:/PRJ-RPL/...).
# This path is STRICTLY READ-ONLY. We never write or edit anything under
# /Volumes/PRJ-RPL/ or Z:\PRJ-RPL\. All outputs go to WORK_ROOT only.
#
_IS_MAC = platform.system() == "Darwin"
_PACKAGE_ROOT = Path(__file__).resolve().parent
_PROJECT_ROOT = _PACKAGE_ROOT.parent.parent

if _IS_MAC:
    MATLAB_BIN = "/Applications/MATLAB_R2025b.app/bin/matlab"
    BASE_DIR = "/Volumes/PRJ-RPL/2RESEARCH/1_ClinicalData"
    WORK_ROOT = str(_PROJECT_ROOT / "Runs")
    MATLAB_CODE_DIR = str(_PROJECT_ROOT / "matlab_files")
    MATLAB_EXTRA_DIR = str(_PROJECT_ROOT / "matlab_files")
    MATLAB_FUNCTIONALITIES_DIR = str(_PROJECT_ROOT / "MATLAB_FUNCTIONALITIES")
else:
    # --- Windows Configuration (User-specified Laptop Paths) ---
    MATLAB_BIN = "C:/Program Files/MATLAB/R2025b/bin/matlab.exe"
    
    # User-provided data directory
    BASE_DIR = "Z:/PRJ-RPL/2RESEARCH/1_ClinicalData"
    
    # MATLAB paths specified by the user
    MATLAB_CODE_DIR = str(_PROJECT_ROOT / "matlab_files")
    MATLAB_EXTRA_DIR = str(_PROJECT_ROOT / "MatlabFolder")
    MATLAB_FUNCTIONALITIES_DIR = str(_PROJECT_ROOT / "MATLAB_FUNCTIONALITIES")
    
    # Use local Runs folder to avoid PermissionError on network drives
    WORK_ROOT = str(_PROJECT_ROOT / "Runs")
    
    # Ensure local directories exist
    Path(WORK_ROOT).mkdir(parents=True, exist_ok=True)
    
    # Create local data/ folder as fallback/placeholder if needed
    _local_data = _PROJECT_ROOT / "data"
    _local_data.mkdir(parents=True, exist_ok=True)

BASE_API = "http://10.65.67.207:8090"


def start_matlab_generate(rt_ct_pres_list, work_root, 
                          skip_copy, skip_dicom2mha, skip_downsample,
                          skip_drr, skip_compress,
                          include_2d_dvf,
                          do_3d_low, do_3d_full, 
                          skip_kv_preprocess,
                          log_file, *,
                          mode: str, start_at: str) -> subprocess.Popen:
    """Launch MATLAB subprocess to execute pipeline."""
    batch = build_matlab_batch(
        rt_ct_pres_list, str(work_root),
        skip_copy, skip_dicom2mha, skip_downsample,
        skip_drr, skip_compress,
        include_2d_dvf, do_3d_low, do_3d_full,
        skip_kv_preprocess,
        str(log_file), mode, start_at
    )
    
    cmd = [MATLAB_BIN, "-batch", batch]

    env = os.environ.copy()
    
    # Ensure MATLAB bin directories are in PATH for DLL resolution
    matlab_bin_dir = Path(MATLAB_BIN).parent
    matlab_win64_dir = matlab_bin_dir / "win64"
    if matlab_win64_dir.exists():
        path = env.get("PATH", "")
        # Add win64 first as it contains critical DLLs like libmvm.dll
        env["PATH"] = f"{matlab_win64_dir}{os.pathsep}{matlab_bin_dir}{os.pathsep}{path}"

    prefdir = work_root / "_matlab_prefs"
    prefdir.mkdir(parents=True, exist_ok=True)
    env["MATLAB_PREFDIR"] = str(prefdir)

    creationflags = 0
    popen_extras = {}
    if platform.system().lower().startswith("win"):
        creationflags = (
            getattr(subprocess, "CREATE_NO_WINDOW", 0)
            | getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0)
        )
    else:
        popen_extras["start_new_session"] = True

    return subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
        creationflags=creationflags,
        cwd=str(work_root),
        env=env,
        **popen_extras
    )


def _normalize_patient_id(pid: str) -> str:
    """Convert things like 'VALKIM_1' or 'VALKIM-01' to 'VALKIM_001'."""
    m = re.match(r"^([A-Za-z]+(?:_[A-Za-z]+)?)[-_]?(\d+)$", pid.strip())
    if not m:
        return pid.strip()
    prefix, num = m.groups()
    return f"{prefix}_{int(num):03d}"


def resolve_rt_ct_pres(trial: str, centre: str, selected_patient_ids: list[str]) -> list[str]:
    """Look up rt_ct_pres for each selected patient via the prescriptions API."""
    if requests is None:
        raise RuntimeError("The 'requests' package is required. Run: pip install requests")

    params = {"trial": trial}
    if centre and centre != "All centres":
        params["centre"] = centre

    r = requests.get(f"{BASE_API}/prescriptions", params=params, timeout=30)
    r.raise_for_status()
    details = r.json()
    pres = details.get("prescriptions", [])

    index = {p.get("patient_trial_id"): p.get("rt_ct_pres") for p in pres if p.get("patient_trial_id")}

    out: list[str] = []
    for pid in selected_patient_ids:
        key = pid
        if key not in index:
            key = _normalize_patient_id(pid)
        if key in index and index[key]:
            out.append(index[key])
    return out


def _esc_matlab(s: str) -> str:
    return s.replace("\\", "/").replace("'", "''")


def build_matlab_batch(rt_ct_pres_list: list[str], work_root: str,
                       skip_copy: bool, skip_dicom2mha: bool, skip_downsample: bool,
                       skip_drr: bool, skip_compress: bool,
                       include_2d_dvf: bool, do_3d_low: bool, do_3d_full: bool,
                       skip_kv_preprocess: bool,
                       log_file: str, mode: str, start_at: str) -> str:
    """Build MATLAB batch command string."""
    # Normalize StartAt to ASCII
    start_at_ascii = {
        "DICOMâ†'MHA": "DICOM2MHA",
        "DICOM â†' MHA": "DICOM2MHA",
        "DICOM2MHA": "DICOM2MHA",
        "Downsample": "Downsample",
        "DRR": "DRR",
        "Compress": "Compress",
        "DVF2D": "DVF2D",
        "DVF3DLow": "DVF3DLow",
        "DVF3DFull": "DVF3DFull",
    }.get(start_at, start_at) if start_at else ""

    rt_literal = "{" + ",".join("'" + _esc_matlab(p) + "'" for p in rt_ct_pres_list) + "}"

    args = [
        f"'LogFile','{_esc_matlab(log_file)}'",
        f"'SkipCopy',{str(skip_copy).lower()}",
        f"'SkipDICOM2MHA',{str(skip_dicom2mha).lower()}",
        f"'SkipDownsample',{str(skip_downsample).lower()}",
        f"'SkipDRR',{str(skip_drr).lower()}",
        f"'SkipCompress',{str(skip_compress).lower()}",
        f"'Skip2DDVF',{str(not include_2d_dvf).lower()}",
        f"'Skip3DLow',{str(not do_3d_low).lower()}",
        f"'Skip3DFull',{str(not do_3d_full).lower()}",
        f"'SkipKVPreprocess',{str(skip_kv_preprocess).lower()}",
        f"'Mode','{_esc_matlab(mode)}'",
    ]
    
    if start_at_ascii:
        args.append(f"'StartAt','{_esc_matlab(start_at_ascii)}'")

    named = ", ".join(args)

    return (
        "restoredefaultpath; try, matlabrc; catch, end; rehash toolboxcache; "
        f"addpath(genpath('{_esc_matlab(MATLAB_CODE_DIR)}')); "
        f"addpath(genpath('{_esc_matlab(MATLAB_EXTRA_DIR)}')); "
        f"addpath(genpath('{_esc_matlab(MATLAB_FUNCTIONALITIES_DIR)}')); "
        f"if exist('generate_voxelmap_training','file')~=2, fprintf(2,'FATAL: generate_voxelmap_training.m not on path\\n'); exit(1); end; "
        f"if ~exist('{_esc_matlab(work_root)}','dir'), mkdir('{_esc_matlab(work_root)}'); end; "
        "try; "
        f"generate_voxelmap_training('{_esc_matlab(BASE_DIR)}', {rt_literal}, "
        f"'{_esc_matlab(work_root)}', {named}); "
        "catch e; disp(getReport(e,'extended')); exit(1); end; "
        "exit(0);"
    )