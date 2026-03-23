# Python Implementation Plan
1. Load the corresponding `.mha` volume using `SimpleITK.ReadImage()`.
2. Determine the target physical spacing or grid dimensions required for downsampling.
3. Configure `sitk.ResampleImageFilter` with the new sizing and spacing parameters.
4. Select an appropriate interpolator (e.g., `sitk.sitkLinear` or `sitk.sitkBSpline` for continuous data).
5. Execute the filter and save the newly resized downsampled volume using `sitk.WriteImage()`.
