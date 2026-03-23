# Python Implementation Plan
1. Load the fixed (DRR) and moving (actual kV projection) images using `SimpleITK`.
2. Set up an `sitk.ImageRegistrationMethod` configured for 2D images.
3. Use a B-Spline or Demons deformable transform to model the 2D Deformation Vector Field (DVF).
4. Optimize using mutual information or mean squares, depending on contrast matching.
5. Extract the displacement field from the transform and save via `sitk.WriteImage()`.
