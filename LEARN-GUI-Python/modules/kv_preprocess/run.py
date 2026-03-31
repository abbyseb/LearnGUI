import importlib
from pathlib import Path

def run(rt_list: list, base_dir: str, patient_root: Path, dataset_type: str = "clinical", **kwargs):
    """
    Super Script for kV preprocessing.
    Dispatches to dataset-specific implementations.
    """
    try:
        module_name = f".implementations.{dataset_type}"
        impl = importlib.import_module(module_name, package=__package__)
    except ImportError:
        impl = importlib.import_module(".implementations.clinical", package=__package__)
    
    return impl.run(rt_list, base_dir, patient_root, **kwargs)
