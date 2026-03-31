import importlib
from pathlib import Path

def run(fixed_path: Path, moving_path: Path, param_path: Path, output_path: Path, dataset_type: str = "clinical", **kwargs):
    """
    Super Script for 3D DVF generation.
    Dispatches to dataset-specific implementations.
    """
    try:
        module_name = f".implementations.{dataset_type}"
        impl = importlib.import_module(module_name, package=__package__)
    except ImportError:
        impl = importlib.import_module(".implementations.clinical", package=__package__)
    
    return impl.run(fixed_path, moving_path, param_path, output_path, **kwargs)
