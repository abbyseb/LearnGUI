"""Compress DRR projections to binary format (pure Python).

PNG files (uint8 display-ready from forward_project): converted to log-domain
via log(256/(P+1)) before downsampling, producing values compatible with the
MATLAB pipeline's compressProj.m output.

TIF/TIFF files (uint16 raw intensity): use the original formula log(65536/(P+1)).
"""
from pathlib import Path
import numpy as np


def process_directory(train_dir: Path, on_batch_done=None) -> None:
    """Convert projection images to 128x128 float32 binary files.
    on_batch_done: optional callback(done, total) called periodically.
    """
    train_dir = Path(train_dir)
    proj_files = (
        list(train_dir.glob("*_Proj_*.tif"))
        + list(train_dir.glob("*_Proj_*.tiff"))
        + list(train_dir.glob("*_Proj_*.png"))
    )
    proj_files.sort(key=lambda p: (p.name,))
    total = len(proj_files)
    # Frequent progress so the UI/log do not look frozen between 120-file boundaries.
    report_every = 25

    for i, proj_path in enumerate(proj_files):
        try:
            img = _read_image(proj_path)
            is_png = proj_path.suffix.lower() == ".png"
            p = img.astype(np.float32) + 1.0

            if is_png:
                p = np.log(256.0 / p)
            else:
                p = np.log(65536.0 / p)

            from scipy.ndimage import zoom
            h, w = p.shape
            p_small = zoom(p, (128 / h, 128 / w), order=1)
            p_flat = p_small[:128, :128].astype(np.float32).ravel(order="F")
            out_path = train_dir / (proj_path.stem + ".bin")
            p_flat.tofile(out_path)
            proj_path.unlink()
        except Exception as e:
            print(f"Skip {proj_path.name}: {e}")
        if callable(on_batch_done) and (
            (i + 1) % report_every == 0 or i + 1 == total
        ):
            on_batch_done(i + 1, total)


def _read_image(p: Path) -> np.ndarray:
    if p.suffix.lower() == ".png":
        from PIL import Image
        return np.array(Image.open(p).convert("L"))
    try:
        import SimpleITK as sitk
        arr = sitk.GetArrayFromImage(sitk.ReadImage(str(p)))
        return arr[0] if arr.ndim == 3 else arr
    except Exception:
        pass
    from PIL import Image
    return np.array(Image.open(p).convert("L"))
