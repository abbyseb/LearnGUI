# backend.py - STANDALONE PYTHON VERSION
import re
from pathlib import Path
import platform, os

try:
    import requests
except ImportError:
    requests = None
    
try:
    from scipy.io import loadmat
except ImportError:
    loadmat = None

_PACKAGE_ROOT = Path(__file__).resolve().parent  # LEARN-GUI-Python/gui
_PROJECT_ROOT = _PACKAGE_ROOT.parent             # LEARN-GUI-Python

# Standalone configuration
# All data and runs are local to the project root for standalone operation by default
_DEFAULT_DATA_DIR = str(_PROJECT_ROOT / "data")
BASE_DIR = _DEFAULT_DATA_DIR
WORK_ROOT = str(_PROJECT_ROOT / "Runs")

def set_data_root(new_root: str):
    """Update the global BASE_DIR used for source data resolution."""
    global BASE_DIR
    if new_root and os.path.isdir(new_root):
        BASE_DIR = new_root
        return True
    return False


# Ensure directories exist
Path(BASE_DIR).mkdir(parents=True, exist_ok=True)
Path(WORK_ROOT).mkdir(parents=True, exist_ok=True)

# API Configuration (same as original)
BASE_API = "http://10.65.67.207:8090"

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

    try:
        r = requests.get(f"{BASE_API}/prescriptions", params=params, timeout=10)
        r.raise_for_status()
        details = r.json()
        pres = details.get("prescriptions", [])
        index = {p.get("patient_trial_id"): p.get("rt_ct_pres") for p in pres if p.get("patient_trial_id")}
    except Exception as e:
        print(f"API Error: {e}")
        return []

    out: list[str] = []
    for pid in selected_patient_ids:
        key = pid
        if key not in index:
            key = _normalize_patient_id(pid)
        if key in index and index[key]:
            out.append(index[key])
    return out