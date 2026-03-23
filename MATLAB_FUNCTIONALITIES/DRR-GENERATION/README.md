# DRR Projection Generation (RTK Integration)

This component generates 2D Digitally Reconstructed Radiographs (DRRs) from 3D CT volumes by simulating X-ray projections.

## Purpose
- Simulate portal images (EPID/kV) for registration training.
- Project 3D anatomy into 2D views from specified gantry angles.
- High-performance forward projection using the Reconstruction Toolkit (RTK).

## Key Scripts and Files
- **`getVALKIMprojections.m`**: Main orchestrator for forward projection. Manages file I/O and external binary calls.
- **`RTKGeometry_360.xml`**: XML configuration file defining the 360-degree projection geometry (120 projections).
- **`RTKGeometry.xml`**: Original geometry file (40 projections).
- **`rtkforwardprojections.exe`**: Compiled high-performance forward projector from the RTK library.
- **`ReadAnglesFromRTKGeometry.m`**: Utility script to parse the geometry XML and extract the list of gantry angles.
- **`MhaRead.m`**: Top-level function for reading MetaImage (.mha) volumes into MATLAB arrays.
- **`mha_read_header.m`**: Specialized parser for MetaImage header files.
- **`mha_read_volume.m`**: Logic for reading raw binary image data from MetaImage files.
- **`seconds2human.m`**: Performance monitoring utility that converts execution time into human-readable format.

## Supplemental Tools
- **`imshow_hnc.m`** (located in parent directory): A utility script to visualize raw `.hnc` projection files directly in MATLAB, useful for quality control.
