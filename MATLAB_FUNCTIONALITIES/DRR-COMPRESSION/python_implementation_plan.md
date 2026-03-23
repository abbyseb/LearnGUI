# Python Implementation Plan
1. Parse the Varian `.hnc` or raw projection files using standard Python binary I/O (`struct`).
2. Read the 2D detector array into a `numpy.ndarray` and reshape to original dimensions.
3. Apply log-attenuation physics transformations to convert raw transmission counts if needed.
4. Use `scipy.ndimage.zoom` or `cv2.resize` to downscale the image to the 128x128 array.
5. Save the flattened 1D array as floats using `numpy.ndarray.tofile()` with `.bin` extension.
