# Python Implementation Plan
1. Use `pydicom` to read the directory of DICOM files and extract metadata (spacing, origin, direction).
2. Use `SimpleITK` (`sitk.ReadImage(sitk.ImageSeriesReader.GetGDCMSeriesFileNames(dir))`) to load the volume.
3. Apply necessary voxel value scaling or slope/intercept adjustments as per DICOM tags.
4. Construct a 3D `sitk.Image` with the correct spatial metadata.
5. Use `sitk.WriteImage` to save the volume directly to `.mha` format.
