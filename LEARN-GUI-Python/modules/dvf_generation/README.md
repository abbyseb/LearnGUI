# dvf_generation Module

This module generates 3D Displacement Vector Fields (DVF) to estimate internal anatomy motion between different CT phases.

## Files
- **`generate.py`**: The core execution script using ITK-Elastix. It performs 3D-to-3D deformable registration and exports the resulting deformation field.
- **`parameters.txt` / `Elastix_BSpline_Sliding.txt`**: Parameter files for the Elastix registration engine. They define the multi-resolution pyramid, metric (Mattes Mutual Information), and transform type (Recursive BSpline).

## Methodology
The DVF generation follows a standardized clinical workflow:
1. **Deformable Registration**: Uses `itk.ElastixRegistrationMethod` to register a "moving" CT phase to a "fixed" (usually phase 6) CT phase.
2. **BSpline Transform**: Employs a multi-resolution BSpline transform to capture complex, non-rigid motion like breathing.
3. **DVF Export**: Utilizes `itk.TransformixFilter` to compute the dense deformation field from the registration's transform parameters.
4. **Coordinate Parity**: The input volumes are typically the `sub_CT_XX.mha` files (low-res). Because these volumes have their origin reset to `(0,0,0)`, the resulting DVF is also defined in unit-grid coordinates, allowing for easy overlay in the GUI.

## Pipeline Integration
- **Bigger Picture**: The generated DVFs are the "ground truth" motion descriptors used to train the clinical tracking models and validate 3D reconstruction accuracy.
- **GUI Connection**: Triggered via the "DVF3D_LOW" or "DVF3D_FULL" steps. The results are automatically loaded into the DVF viewer monitor for clinical review.
- **Pre-requisites**: Two 3D CT volumes (`.mha`) and a valid Elastix parameter file.
- **Expected Next Step**: The resulting DVF files (`.mha`) are stored in the patient's `train/` directory for motion modeling.
