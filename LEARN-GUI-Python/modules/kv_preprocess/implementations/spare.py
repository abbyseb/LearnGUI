import shutil
import logging
from pathlib import Path

def run(rt_list: list, base_dir: str, patient_root: Path, **kwargs):
    """
    SPARE implementation for kV preprocessing.
    Skips clinical RT/test data logic, ensures source.mha exists.
    """
    logging.info("SPARE kV Preprocess: Ensuring source.mha")
    
    train_dir = patient_root / "train"
    if not train_dir.exists():
        return False
        
    # In SPARE, CT_06.mha is the reference.
    ct06 = train_dir / "CT_06.mha"
    source_mha = train_dir / "source.mha"
    
    if ct06.exists() and not source_mha.exists():
        logging.info(f"SPARE kV Preprocess: copying {ct06.name} to {source_mha.name}")
        shutil.copy2(ct06, source_mha)
    elif not ct06.exists():
        logging.warning("SPARE kV Preprocess: CT_06.mha not found!")
        
    return True
