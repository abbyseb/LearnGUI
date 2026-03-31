import shutil
from pathlib import Path
from ..process import copy_test_data, process_kv_images

def run(rt_list: list, base_dir: str, patient_root: Path, **kwargs):
    """
    Standard clinical implementation: calls the existing kV preprocessing logic.
    """
    if rt_list:
        copy_test_data(rt_list[0], base_dir, patient_root)
    
    test_dir = patient_root / "test"
    if test_dir.exists():
        process_kv_images(test_dir)
    
    # Copy source.mha (exhale reference)
    train_dir = patient_root / "train"
    ct06 = train_dir / "CT_06.mha"
    if ct06.exists():
        shutil.copy2(ct06, train_dir / "source.mha")
        
    return True
