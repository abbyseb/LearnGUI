"""MhaWrite.m — uncompressed MetaImage float32, MATLAB column-major order."""

from __future__ import annotations

from pathlib import Path

import numpy as np


def write_mha_float32(volume: np.ndarray, pixel_dimensions_xyz: np.ndarray, path: Path) -> None:
    """
    Write 3D float32 MetaImage matching MhaWrite.m / ITK expectations.

    ``volume`` axes are the same as MATLAB ``M`` after IEC remaps (DimSize order = shape).
    Binary payload is column-major (first index varies fastest), as ``fwrite(...,'float')`` in MATLAB.
    """
    path = Path(path)
    vol = np.asfortranarray(np.asarray(volume, dtype=np.float32))
    d = np.array(vol.shape, dtype=np.float64)
    pix = np.asarray(pixel_dimensions_xyz, dtype=np.float64).reshape(3)
    offset = -0.5 * (d - 1.0) * pix
    header = (
        "ObjectType = Image\n"
        "NDims = 3\n"
        "BinaryData = True\n"
        "BinaryDataByteOrderMSB = False\n"
        "CompressedData = False\n"
        "TransformMatrix = 1 0 0 0 1 0 0 0 1\n"
        f"Offset = {offset[0]} {offset[1]} {offset[2]}\n"
        "CenterOfRotation = 0 0 0\n"
        "AnatomicalOrientation = RAI\n"
        f"ElementSpacing = {pix[0]} {pix[1]} {pix[2]}\n"
        f"DimSize = {int(d[0])} {int(d[1])} {int(d[2])}\n"
        "ElementType = MET_FLOAT\n"
        "ElementDataFile = LOCAL\n"
    )
    path.write_bytes(header.encode("ascii") + vol.tobytes(order="F"))
