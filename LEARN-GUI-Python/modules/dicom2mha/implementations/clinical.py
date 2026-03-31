from pathlib import Path
from ..convert import convert_dicom_to_mha

def run(input_dir: Path, output_dir: Path, **kwargs):
    """
    Standard clinical implementation: calls the existing convert_dicom_to_mha.
    """
    return convert_dicom_to_mha(input_dir, output_dir, **kwargs)
