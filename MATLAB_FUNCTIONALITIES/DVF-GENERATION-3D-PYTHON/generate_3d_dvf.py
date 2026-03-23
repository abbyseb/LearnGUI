import itk
import os
import time

def generate_dvf(fixed_path, moving_path, parameter_path, output_path):
    print(f"Registering {moving_path} to {fixed_path}...")
    
    # 1. Load Images
    fixed_image = itk.imread(fixed_path, itk.F)
    moving_image = itk.imread(moving_path, itk.F)
    
    # 2. Setup Parameter Object
    parameter_object = itk.ParameterObject.New()
    parameter_object.AddParameterFile(parameter_path)
    
    # 3. Initialize Elastix
    # Specify image types [Fixed, Moving] to avoid Dispatching issues
    ImageType = type(fixed_image)
    elastix_object = itk.ElastixRegistrationMethod[ImageType, ImageType].New()
    elastix_object.SetFixedImage(fixed_image)
    elastix_object.SetMovingImage(moving_image)
    elastix_object.SetParameterObject(parameter_object)
    
    # Optional: Log to console if needed
    # elastix_object.SetLogToConsole(True)
    
    # 4. Run Registration
    start_time = time.time()
    elastix_object.Update()
    duration = time.time() - start_time
    print(f"Registration completed in {duration:.2f} seconds.")
    
    # 5. Generate Displacement Field (DVF)
    # The registration result contains the transform
    transform_parameter_object = elastix_object.GetTransformParameterObject()
    
    transformix_object = itk.TransformixFilter[ImageType].New()
    transformix_object.SetMovingImage(moving_image) # dummy input for transformix
    transformix_object.SetTransformParameterObject(transform_parameter_object)
    transformix_object.SetComputeDeformationField(True)
    transformix_object.Update()
    
    dvf = transformix_object.GetOutputDeformationField()
    
    # 6. Save DVF
    itk.imwrite(dvf, output_path)
    print(f"DVF saved to {output_path}")

if __name__ == "__main__":
    fixed = "sub_CT_01.mha"
    moving = "sub_CT_06.mha"
    params = "Elastix_BSpline_Sliding.txt"
    output = "DVF_Python_01.mha"
    
    if os.path.exists(fixed) and os.path.exists(moving) and os.path.exists(params):
        generate_dvf(fixed, moving, params, output)
    else:
        print("Error: Missing input files in current directory.")
