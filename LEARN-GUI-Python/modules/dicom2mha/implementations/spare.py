import logging
import os
import shutil
import re
from pathlib import Path

def run(input_dir: Path, output_dir: Path, **kwargs):
    """
    SPARE implementation for DICOM2MHA: 
    Normalizes nomenclature by linking/copying GTVol_XX.mha to CT_XX.mha.
    """
    logging.info(f"SPARE Normalize: Scanning {input_dir}")
    
    # SPARE files are usually: GTVol_01.mha, GTVol_02.mha ...
    # We want them to be CT_01.mha, CT_02.mha ...
    
    mhas = list(input_dir.glob("GTVol_*.mha"))
    if not mhas:
        # Fallback: maybe they are already named CT_*.mha or something else
        logging.info("SPARE Normalize: No GTVol_*.mha found, checking for existing CT_*.mha")
        if list(input_dir.glob("CT_*.mha")):
            return True
        return False

    # Create SeriesInfo.txt for progress tracking in the GUI
    # The format should match what the DICOM2MHAStepManager expects
    series_info = output_dir / "SeriesInfo.txt"
    with series_info.open("w", encoding="utf-8") as f:
        f.write("SPARE Normalization Series Info\n")
        f.write("==============================\n")
        for src in mhas:
            match = re.search(r"GTVol_(\d+)", src.name)
            if match:
                num = match.group(1)
                f.write(f"CT_{num} : {src.name}\n")
    
    # Process normalization
    for src in mhas:
        # Extract number
        match = re.search(r"GTVol_(\d+)", src.name)
        if match:
            num = match.group(1)
            dst = output_dir / f"CT_{num}.mha"
            if not dst.exists():
                logging.info(f"SPARE Normalize: {src.name} -> {dst.name}")
                try:
                    os.link(src, dst) # Hardlink is efficient and safe
                except OSError:
                    shutil.copy2(src, dst)
    
    return True
