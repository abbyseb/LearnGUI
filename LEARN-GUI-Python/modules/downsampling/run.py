import importlib
from pathlib import Path

def run(train_dir: Path, dataset_type: str = "clinical", **kwargs):
    """
    Super Script for downsampling.
    Dispatches to dataset-specific implementations.
    """
    try:
        module_name = f".implementations.{dataset_type}"
        impl = importlib.import_module(module_name, package=__package__)
    except ImportError:
        impl = importlib.import_module(".implementations.clinical", package=__package__)
    
    return impl.run(train_dir, **kwargs)
