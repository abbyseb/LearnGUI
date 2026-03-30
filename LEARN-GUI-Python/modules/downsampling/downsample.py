import itk
import numpy as np
import os
import glob
from typing import Callable, Optional

def downsample_volume(input_path, output_path, target_size=(128, 128, 128)):
    """
    Downsamples a 3D volume to a target size, matching the MATLAB logic.
    Calculates spacing to cover the full physical extent of the input.
    """
    image = itk.imread(str(input_path))
    in_size = itk.size(image)
    in_spacing = image.GetSpacing()
    
    # Calculate spacing to fit in_size*in_spacing into target_size
    out_spacing = [
        (in_size[i] * in_spacing[i]) / target_size[i]
        for i in range(3)
    ]
    
    interpolator = itk.LinearInterpolateImageFunction.New(image)
    resampler = itk.ResampleImageFilter.New(image)
    resampler.SetInterpolator(interpolator)
    resampler.SetSize(target_size)
    resampler.SetOutputOrigin(image.GetOrigin())
    resampler.SetOutputSpacing(out_spacing)
    resampler.SetOutputDirection(image.GetDirection())
    resampler.Update()
    
    resampled_image = resampler.GetOutput()
    arr = itk.GetArrayFromImage(resampled_image)
    arr[arr < 0] = 0
    
    # Match MATLAB downsampleVALKIMvolumes.m:
    # info.Offset = [0, 0, 0]; info.PixelDimensions = [1, 1, 1];
    # We overwrite the physical metadata so Elastix/Transformix output
    # matches the unit-coordinate system expected by the DVF viewer.
    final_image = itk.GetImageFromArray(arr)
    final_image.SetSpacing([1.0, 1.0, 1.0])
    final_image.SetOrigin([0, 0, 0])
    final_image.SetDirection(itk.matrix_from_array(np.eye(3)))
    
    itk.imwrite(final_image, str(output_path))
    print(f"Saved downsampled volume to {output_path}")

def process_directory(
    directory,
    on_volume_done: Optional[Callable[[int, int, str], None]] = None,
):
    """
    Downsample each CT_*.mha in directory to sub_CT_XX.mha.
    Optional on_volume_done(done_index, total, output_basename) for GUI/workers.
    """
    directory = os.path.normpath(str(directory))
    files = sorted(glob.glob(os.path.join(directory, "CT_*.mha")))
    total = len(files)
    for i, f in enumerate(files, start=1):
        base = os.path.basename(f)
        out_name = f"sub_CT{base[-7:]}"
        out_path = os.path.join(directory, out_name)
        downsample_volume(f, out_path)
        if on_volume_done is not None:
            on_volume_done(i, total, out_name)

if __name__ == "__main__":
    process_directory(".")
