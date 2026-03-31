from pathlib import Path
from ..compress import process_directory

def run(train_dir: Path, **kwargs):
    """
    Standard clinical implementation: calls the existing process_directory.
    """
    return process_directory(train_dir, **kwargs)
