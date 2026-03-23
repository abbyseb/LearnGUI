# [NEW] Python Implementation Plan (itk-rtk)

Replace the `rtkforwardprojections.exe` dependency with a native Python script using `itk-rtk`:

1. **Load Geometry**: Use `rtk.ThreeDCircularProjectionGeometryXMLFileReader` to parse your `RTKGeometry_360.xml`.
2. **Load Volume**: Read the downsampled MetaImage (`.mha`) using `itk.imread()`.
3. **Configure Filter**: Initialize `rtk.JosephForwardProjectionImageFilter`, connecting the volume and the geometry object.
4. **Transform Output**: Convert the output attenuation maps ($x$) to intensity values ($I = I_0 e^{-x}$) and scale to `uint16` (0-65535).
5. **Save Projections**: Write to `.hnc` or `.mha` using `itk.imwrite` or manual binary writing for HNC compatibility.

### Example Code Snippet:
```python
import itk
from itk import RTK as rtk

# Load 360 geometry
rtk_reader = rtk.ThreeDCircularProjectionGeometryXMLFileReader.New()
rtk_reader.SetFilename("RTKGeometry_360.xml")
rtk_reader.GenerateOutputInformation()
geometry = rtk_reader.GetOutput()

# Setup Forward Projector
filter = rtk.JosephForwardProjectionImageFilter.New()
filter.SetInput(itk.imread("sub_CT_01.mha"))
filter.SetGeometry(geometry)
filter.Update()
```
