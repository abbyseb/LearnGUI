import importlib
from pathlib import Path

def run(moving_image_path: str, output_dir: str, dataset_type: str = "clinical", **kwargs):
    """
    Super Script for DRR generation.
    Dispatches to dataset-specific implementations.
    """
    try:
        module_name = f".implementations.{dataset_type}"
        impl = importlib.import_module(module_name, package=__package__)
    except ImportError:
        impl = importlib.import_module(".implementations.clinical", package=__package__)
    
    return impl.run(moving_image_path, output_dir, **kwargs)
