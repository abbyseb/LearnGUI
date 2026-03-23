import itk
import numpy as np
import os
import subprocess
import csv
import time

def run_python_registration(fixed_path, moving_path, parameter_path, output_path):
    print("--- Running Python itk-elastix Registration (Silent) ---")
    fixed_image = itk.imread(fixed_path, itk.F)
    moving_image = itk.imread(moving_path, itk.F)
    parameter_object = itk.ParameterObject.New()
    parameter_object.AddParameterFile(parameter_path)
    
    ImageType = type(fixed_image)
    elastix_object = itk.ElastixRegistrationMethod[ImageType, ImageType].New()
    elastix_object.SetFixedImage(fixed_image)
    elastix_object.SetMovingImage(moving_image)
    elastix_object.SetParameterObject(parameter_object)
    elastix_object.SetLogToConsole(False) # Suppress verbose output
    
    start = time.time()
    elastix_object.Update()
    
    transform_parameter_object = elastix_object.GetTransformParameterObject()
    transformix_object = itk.TransformixFilter[ImageType].New()
    transformix_object.SetMovingImage(moving_image)
    transformix_object.SetTransformParameterObject(transform_parameter_object)
    transformix_object.SetComputeDeformationField(True)
    transformix_object.SetLogToConsole(False) # Suppress verbose output
    transformix_object.Update()
    
    dvf = transformix_object.GetOutputDeformationField()
    itk.imwrite(dvf, output_path)
    print(f"Python DVF saved to {output_path} (Time: {time.time()-start:.2f}s)")

def run_exe_registration(fixed_path, moving_path, parameter_path, output_path):
    print("--- Running original elastix.exe Registration (Silent) ---")
    # Get the directory where this script is located
    base_dir = os.path.dirname(os.path.abspath(__file__))
    
    out_dir = os.path.join(base_dir, "out_exe")
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)
        
    elastix_bin = os.path.join(base_dir, "elastix.exe")
    transformix_bin = os.path.join(base_dir, "transformix.exe")
    
    if not os.path.exists(elastix_bin):
        print(f"Error: {elastix_bin} not found in {base_dir}")
        return False

    start = time.time()
    try:
        print(f"Running: {elastix_bin} -f {fixed_path} ...")
        # Wrap all paths in quotes to handle spaces in folder names
        cmd_elastix = f'"{elastix_bin}" -f "{fixed_path}" -m "{moving_path}" -p "{parameter_path}" -out "{out_dir}"'
        
        # Run with shell=True and suppress output
        subprocess.run(cmd_elastix, check=True, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
        # 2. Transformation
        tp_file = os.path.join(out_dir, "TransformParameters.0.txt")
        print(f"Running: {transformix_bin} -tp {tp_file} ...")
        cmd_transformix = f'"{transformix_bin}" -tp "{tp_file}" -def all -out "{out_dir}"'
        subprocess.run(cmd_transformix, check=True, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
        # 3. Rename output
        res_file = os.path.join(out_dir, "deformationField.mha")
        if os.path.exists(res_file):
            import shutil
            shutil.copy(res_file, output_path)
            print(f"EXE DVF saved to {output_path} (Time: {time.time()-start:.2f}s)")
            return True
        else:
            print(f"Error: {res_file} not found after exe run.")
            return False
    except Exception as e:
        print(f"Error running EXE: {e}")
        return False

def compare_dvfs(path_a, path_b, output_csv):
    print("--- Comparing DVF Outputs ---")
    dvf_a = itk.imread(path_a)
    dvf_b = itk.imread(path_b)
    arr_a = itk.GetArrayFromImage(dvf_a)
    arr_b = itk.GetArrayFromImage(dvf_b)
    
    diff = arr_a - arr_b
    mse = np.mean(diff**2)
    rmse = np.sqrt(mse)
    
    results = [
        ["Metric", "Value"],
        ["RMSE", f"{rmse:.8f}"],
        ["MSE", f"{mse:.8f}"],
        ["Status", "SUCCESS" if rmse < 1e-4 else "NOTICEABLE DIFFERENCE"]
    ]
    
    with open(output_csv, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(results)
    
    print(f"Comparison report saved to {output_csv}")
    print(f"  RMSE: {rmse:.8f}")

if __name__ == "__main__":
    # Get the directory where this script is located
    base_dir = os.path.dirname(os.path.abspath(__file__))
    
    fixed = os.path.join(base_dir, "sub_CT_01.mha")
    moving = os.path.join(base_dir, "sub_CT_06.mha")
    params = os.path.join(base_dir, "Elastix_BSpline_Sliding.txt")
    
    # 1. Run EXE
    exe_success = run_exe_registration(fixed, moving, params, os.path.join(base_dir, "DVF_EXE.mha"))
    
    # 2. Run Python
    run_python_registration(fixed, moving, params, os.path.join(base_dir, "DVF_PYTHON.mha"))
    
    # 3. Compare if both worked
    python_dvf = os.path.join(base_dir, "DVF_PYTHON.mha")
    exe_dvf = os.path.join(base_dir, "DVF_EXE.mha")
    report_csv = os.path.join(base_dir, "final_verification_report.csv")
    
    if exe_success and os.path.exists(python_dvf):
        compare_dvfs(python_dvf, exe_dvf, report_csv)
    else:
        print("\nNote: EXE registration failed (typical for some virtual environments).")
        print("Please run this script on your local workstation to perform the cross-verification.")
