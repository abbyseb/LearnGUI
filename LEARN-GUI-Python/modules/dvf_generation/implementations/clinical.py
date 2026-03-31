from pathlib import Path
from ..generate import generate_dvf

def run(fixed_path: Path, moving_path: Path, param_path: Path, output_path: Path, **kwargs):
    """
    Standard clinical implementation: calls the existing generate_dvf.
    """
    return generate_dvf(fixed_path, moving_path, param_path, output_path, **kwargs)
