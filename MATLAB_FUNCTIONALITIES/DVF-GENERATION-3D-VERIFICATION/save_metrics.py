import itk
import numpy as np
import csv
import os

def save_dvf_metrics(dvf_path, output_csv):
    if not os.path.exists(dvf_path):
        print(f"Error: {dvf_path} not found.")
        return
        
    dvf = itk.imread(dvf_path)
    arr = itk.GetArrayFromImage(dvf)
    # Norm magnitude along the last axis [x,y,z]
    mag = np.linalg.norm(arr, axis=-1)
    
    results = [
        ["Metric", "Value"],
        ["Shape", str(arr.shape)],
        ["Dimensions", str(dvf.GetImageDimension())],
        ["Min Magnitude", f"{np.min(mag):.6f}"],
        ["Max Magnitude", f"{np.max(mag):.6f}"],
        ["Mean Magnitude", f"{np.mean(mag):.6f}"],
        ["Standard Deviation", f"{np.std(mag):.6f}"]
    ]
    
    with open(output_csv, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerows(results)
    
    print(f"Verification metrics saved to {output_csv}")

if __name__ == "__main__":
    save_dvf_metrics("DVF_Python_01.mha", "dvf_verification_results.csv")
