import itk
import numpy as np
import os
import glob

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

def process_directory(directory):
    files = glob.glob(os.path.join(directory, "CT_*.mha"))
    for f in files:
        base = os.path.basename(f)
        # Match MATLAB name: sub_CT + end parts of name
        out_name = f"sub_CT{base[-7:]}"
        downsample_volume(f, os.path.join(directory, out_name))

if __name__ == "__main__":
    process_directory(".")
