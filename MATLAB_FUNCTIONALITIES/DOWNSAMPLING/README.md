# volume Downsampling

This component reduces the resolution of CT and mask volumes to optimize for deep learning training and memory constraints.

## Purpose
- Standardize volume dimensions (typically to 128x128x128).
- Maintain correct voxel sizes (spacing) and origin after downsampling.
- Improve training performance by reducing input size for neural networks.

## Key Scripts
- **`downsampleVALKIMvolumes.m`**: Main logic for cubic interpolation on CT data and nearest-neighbor interpolation on binary masks (Body, Lungs, GTV).

## Outputs
- `sub_CT_XX.mha`: Downsampled CT volume.
- `sub_Lungs_XX.mha`: Downsampled Lung mask.
- `sub_GTV_XX.mha`: Downsampled GTV mask.
