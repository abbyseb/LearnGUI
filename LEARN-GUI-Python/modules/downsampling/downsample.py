import itk
import numpy as np
import os
import glob
from typing import Callable, Optional

def downsample_volume(input_path, output_path, target_size=(128, 128, 128)):
    """
    Downsamples a 3D volume to a target size, matching the MATLAB logic.
    """
    image = itk.imread(input_path)
    
    # In MATLAB: imresize3 to [512, 256, 512] then slice 1:4, 1:2, 1:4 -> [128, 128, 128]
    # We can directly resample to [128, 128, 128] using ITK
    
    # Calculate new spacing to reach target size [128, 128, 128] 
    # but the MATLAB code explicitly sets info.PixelDimensions = [1, 1, 1]
    
    interpolator = itk.LinearInterpolateImageFunction.New(image)
    
    resampler = itk.ResampleImageFilter.New(image)
    resampler.SetInterpolator(interpolator)
    resampler.SetSize(target_size)
    resampler.SetOutputOrigin([0, 0, 0])
    resampler.SetOutputSpacing([1.0, 1.0, 1.0])
    resampler.SetOutputDirection(itk.matrix_from_array(np.eye(3)))
    
    # Match the MATLAB behavior of clipping negative values (if any)
    resampled_image = resampler.GetOutput()
    resampler.Update()
    
    arr = itk.GetArrayFromImage(resampled_image)
    arr[arr < 0] = 0
    
    final_image = itk.GetImageFromArray(arr)
    final_image.SetSpacing([1.0, 1.0, 1.0])
    final_image.SetOrigin([0, 0, 0])
    
    itk.imwrite(final_image, output_path)
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
