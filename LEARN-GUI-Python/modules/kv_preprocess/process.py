"""kV image preprocessing: TIFF to binary (pure Python)."""
from pathlib import Path
import shutil

def copy_test_data(rt_ct_pres: str, base_dir: str, patient_root: Path) -> None:
    """Copy Treatment files (test data) to patient_root/test."""
    parts = [p for p in (rt_ct_pres or "").strip("/").split("/") if p]
    for marker in ("1_ClinicalData", "PRJ-RPL"):
        if marker in parts:
            idx = parts.index(marker)
            if marker == "1_ClinicalData" and idx + 1 < len(parts):
                parts = parts[idx + 1:]
                break
            if marker == "PRJ-RPL" and idx + 4 <= len(parts):
                parts = parts[idx + 4:]
                break
    if len(parts) >= 3:
        parts[2] = "Treatment files"
    if len(parts) >= 4:
        parts = parts[:3]
    src = Path(base_dir).joinpath(*parts)
    dst = Path(patient_root) / "test"
    if src.exists() and src.is_dir():
        dst.mkdir(parents=True, exist_ok=True)
        for item in src.iterdir():
            shutil.copytree(item, dst / item.name, dirs_exist_ok=True) if item.is_dir() else shutil.copy2(item, dst / item.name)


def process_kv_images(test_dir: Path) -> None:
    """Convert TIFF files in kV folders to .bin format."""
    import numpy as np
    test_dir = Path(test_dir)
    for fx_dir in sorted(test_dir.glob("Fx*")):
        kv_dir = fx_dir / "kV"
        if not kv_dir.is_dir():
            continue
        from itertools import chain
        for tif in chain(kv_dir.glob("*.tif"), kv_dir.glob("*.tiff"), kv_dir.glob("*.png")):
            try:
                from PIL import Image
                img = np.array(Image.open(tif).convert("L")).astype(np.float32)
                out = kv_dir / (tif.stem + ".bin")
                img.tofile(out)
            except Exception as e:
                print(f"Skip {tif.name}: {e}")
