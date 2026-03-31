import importlib
import logging
from pathlib import Path

def run(input_dir: Path, output_dir: Path, dataset_type: str = "clinical", **kwargs):
    """
    Super Script for DICOM to MHA conversion.
    Dispatches to dataset-specific implementations.
    """
    try:
        # Dynamic loading: implementations.<dataset_type>
        module_name = f".implementations.{dataset_type}"
        impl = importlib.import_module(module_name, package=__package__)
    except ImportError:
        # Fallback to clinical if specific implementation not found
        impl = importlib.import_module(".implementations.clinical", package=__package__)
    
    return impl.run(input_dir, output_dir, **kwargs)
