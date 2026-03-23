# Python Implementation Plan
1. Load the already downsampled 3D CT volumes (using the downsampling module) via `SimpleITK`.
2. Initialize `ANTsPy` SyN or `sitk.ImageRegistrationMethod` using a coarse B-Spline grid spacing.
3. Run the deformable image registration on these low-resolution representations to minimize compute.
4. Obtain the calculated coarse 3D deformation tracking grid vectors.
5. Save the resultant low-res 3D DVF `.mha` file, maintaining the downsampled spatial metadata.
