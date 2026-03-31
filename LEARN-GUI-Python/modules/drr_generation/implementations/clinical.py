from pathlib import Path
from ..generate import generate_drrs

def run(moving_image_path: str, output_dir: str, **kwargs):
    """
    Standard clinical implementation: calls the existing generate_drrs.
    """
    return generate_drrs(moving_image_path, output_dir, **kwargs)
