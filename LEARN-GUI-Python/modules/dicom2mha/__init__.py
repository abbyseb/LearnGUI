"""
DICOM → MHA (MATLAB VALKIM / DICOM2IEC parity).

IEC 61217 (beams, couch, collimator, etc.) is **not** applied here as the explicit 3×3 patient
rotation matrix used for RT plans. This package reproduces the MATLAB volume permutations and
MetaImage header only; RT IEC frame transforms for points/plans are a separate step.
"""

from .convert import convert_dicom_to_mha

__all__ = ["convert_dicom_to_mha"]
