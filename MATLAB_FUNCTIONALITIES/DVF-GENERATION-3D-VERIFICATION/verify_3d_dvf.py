import itk
import numpy as np
import os
import csv

def compare_dvfs(path_a, path_b, output_csv):
    print(f"Comparing DVF A: {path_a}")
    print(f"Comparing DVF B: {path_b}")
    
    # 1. Load DVFs
    dvf_a = itk.imread(path_a)
    dvf_b = itk.imread(path_b)
    
    # 2. Get Arrays
    arr_a = itk.GetArrayFromImage(dvf_a)
    arr_b = itk.GetArrayFromImage(dvf_b)
    
    # 3. Compute Metrics
    # DVF is usually (X, Y, Z, 3) where last dim is vector components
    diff = arr_a - arr_b
    mse = np.mean(diff**2)
    rmse = np.sqrt(mse)
    
    # Max displacement difference
    max_diff = np.max(np.abs(diff))
    
    # Mean vector magnitude difference
    mag_a = np.linalg.norm(arr_a, axis=-1)
    mag_b = np.linalg.norm(arr_b, axis=-1)
    mean_mag_diff = np.mean(np.abs(mag_a - mag_b))
    
    print(f"\nVerification Results:")
    print(f"  RMSE: {rmse:.6f}")
    print(f"  MSE:  {mse:.6f}")
    print(f"  Max Diff: {max_diff:.6f}")
    print(f"  Mean Mag Diff: {mean_mag_diff:.6f}")
    
    # 4. Save to CSV
    with open(output_csv, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Metric", "Value"])
        writer.writerow(["RMSE", f"{rmse:.6f}"])
        writer.writerow(["MSE", f"{mse:.6f}"])
        writer.writerow(["MaxDiff", f"{max_diff:.6f}"])
        writer.writerow(["MeanMagDiff", f"{mean_mag_diff:.6f}"])
    
    print(f"Results saved to {output_csv}")

if __name__ == "__main__":
    # Example paths
    dvf_exe = "DVF_EXE_01.mha"
    dvf_python = "DVF_Python_01.mha"
    
    if os.path.exists(dvf_exe) and os.path.exists(dvf_python):
        compare_dvfs(dvf_exe, dvf_python, "dvf_verification_results.csv")
    else:
        print("Error: Missing input DVF files for comparison.")
        print(f"Please copy the outputs from the EXE and PYTHON folders to this directory as {dvf_exe} and {dvf_python}.")
