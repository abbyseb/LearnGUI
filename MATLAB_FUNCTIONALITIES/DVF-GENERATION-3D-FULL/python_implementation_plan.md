# Python Implementation Plan
1. Load the planning CT (fixed) and the reconstructed CBCT or daily volume (moving) as 3D arrays.
2. Utilize `ANTsPy` (Advanced Normalization Tools) or `SimpleITK` elastix for robust 3D registration.
3. Perform a rigid initialization followed by a multi-resolution non-linear SyN (Symmetric Normalization).
4. Extract the dense 3D dense vector field mappings from the registration outputs.
5. Save the 3-component multi-channel 3D displacement volume as `.mha` for downstream AI tasks.
