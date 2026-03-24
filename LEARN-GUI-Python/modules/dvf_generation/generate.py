import itk
import os
import time

def generate_dvf(fixed_path, moving_path, parameter_path, output_path):
    """
    Generates a 3D DVF using Python itk-elastix.
    """
    fixed_image = itk.imread(fixed_path, itk.F)
    moving_image = itk.imread(moving_path, itk.F)
    
    parameter_object = itk.ParameterObject.New()
    parameter_object.AddParameterFile(parameter_path)
    
    ImageType = type(fixed_image)
    elastix_object = itk.ElastixRegistrationMethod[ImageType, ImageType].New()
    elastix_object.SetFixedImage(fixed_image)
    elastix_object.SetMovingImage(moving_image)
    elastix_object.SetParameterObject(parameter_object)
    elastix_object.SetLogToConsole(False)
    
    print(f"Starting itk-elastix registration...")
    start = time.time()
    elastix_object.Update()
    print(f"Registration completed in {time.time() - start:.2f}s")
    
    # Generate the DVF using Transformix
    transform_parameter_object = elastix_object.GetTransformParameterObject()
    transformix_object = itk.TransformixFilter[ImageType].New()
    transformix_object.SetMovingImage(moving_image)
    transformix_object.SetTransformParameterObject(transform_parameter_object)
    transformix_object.SetComputeDeformationField(True)
    transformix_object.SetLogToConsole(False)
    
    transformix_object.Update()
    dvf = transformix_object.GetOutputDeformationField()
    itk.imwrite(dvf, output_path)
    print(f"Saved Python DVF to {output_path}")
    return True

if __name__ == "__main__":
    # Example usage
    generate_dvf("fixed.mha", "moving.mha", "params.txt", "dvf.mha")
