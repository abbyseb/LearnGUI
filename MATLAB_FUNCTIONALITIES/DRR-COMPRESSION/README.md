# DRR Projection Compression

This component compresses high-resolution raw DRR projections into a compact binary format for deep learning applications.

## Purpose
- Reduce dataset storage overhead.
- Resize projections to standard training resolutions (e.g., 128x128).
- Convert proprietary or high-depth formats into standardized floats.

## Key Scripts and Files
- **`compressProj.m`**: Main entry point for projection processing. Converts `.hnc` or `.mha` images into `.bin` format.
- **`HncRead.m`**: Utility script for parsing and reading `.hnc` (High-efficiency NC) projection files.
- **`HndRead.m`**: Utility script for reading Varian `.hnd` log/projection files.
- **`VarianReader.jar`**: Java implementation of the Varian file format parser, used as a dependency for reading clinical logs.
- **`seconds2human.m`**: Performance monitoring utility that converts execution time into human-readable format.

## Outputs
- `XX_Proj_YY.bin`: Compressed projection binary for phase XX at angle YY.
