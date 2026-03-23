import itk
import numpy as np
import os
import subprocess
import csv

def run_exe_projection(geometry_file, input_volume, output_mha):
    """Runs the original rtkforwardprojections.exe"""
    cmd = [
        "rtkforwardprojections.exe",
        "-g", geometry_file,
        "-i", input_volume,
        "-o", output_mha,
        "--dimension", "1024,768",
        "--spacing", "0.388,0.388"
    ]
    print(f"Running EXE: {' '.join(cmd)}")
    subprocess.run(cmd, check=True)

def run_itk_rtk_projection(geometry_file, input_volume, output_mha):
    """Runs projection using itk-rtk Joseph Forward Projector"""
    print("Running ITK-RTK (Native Python)...")
    rtk = itk.RTK
    
    # Load Geometry
    xml_reader = rtk.ThreeDCircularProjectionGeometryXMLFileReader.New()
    xml_reader.SetFilename(geometry_file)
    xml_reader.GenerateOutputInformation()
    geometry = xml_reader.GetOutputObject()
    
    # Load Volume
    ct_image = itk.imread(input_volume, itk.F)
    
    # Create Constant Image for Projections (matching EXE dimensions)
    constant_image_source = rtk.ConstantImageSource[itk.Image[itk.F, 3]].New()
    constant_image_source.SetOrigin([-198.462, -148.798, 0])
    constant_image_source.SetSpacing([0.388, 0.388, 1.0])
    constant_image_source.SetSize([1024, 768, len(geometry.GetGantryAngles())])
    constant_image_source.SetConstant(0.0)
    
    # Joseph Forward Projector
    forward_projector = rtk.JosephForwardProjectionImageFilter[itk.Image[itk.F, 3], itk.Image[itk.F, 3]].New()
    forward_projector.SetInput(0, constant_image_source.GetOutput())
    forward_projector.SetInput(1, ct_image)
    forward_projector.SetGeometry(geometry)
    forward_projector.Update()
    
    itk.imwrite(forward_projector.GetOutput(), output_mha)

def compute_metrics(base_arr, test_arr):
    mse = np.mean((base_arr - test_arr) ** 2)
    rmse = np.sqrt(mse)
    
    # Cosine Similarity (1 - cosine distance)
    # Flatten arrays for vector comparison
    b_flat = base_arr.flatten()
    t_flat = test_arr.flatten()
    
    # Avoid zero division
    norm_product = np.linalg.norm(b_flat) * np.linalg.norm(t_flat)
    if norm_product == 0:
        c_sim = 1.0 if np.all(b_flat == t_flat) else 0.0
    else:
        c_sim = np.dot(b_flat, t_flat) / norm_product
        
    return mse, rmse, c_sim

def main():
    geom = "RTKGeometry_360.xml"
    vol = "CT_01.mha"
    exe_out = "out_exe.mha"
    itk_out = "out_itk.mha"
    csv_file = "drr_verification_results.csv"
    
    if not os.path.exists("rtkforwardprojections.exe"):
        print("Error: rtkforwardprojections.exe not found in current directory.")
        return

    # Generate both
    run_exe_projection(geom, vol, exe_out)
    run_itk_rtk_projection(geom, vol, itk_out)
    
    # Load and compare
    print("Loading results for comparison...")
    exe_img = itk.imread(exe_out)
    itk_img = itk.imread(itk_out)
    
    exe_data = itk.GetArrayFromImage(exe_img)
    itk_data = itk.GetArrayFromImage(itk_img)
    
    num_proj = exe_data.shape[0]
    
    print(f"Comparing {num_proj} projections...")
    
    results = []
    for i in range(num_proj):
        mse, rmse, c_sim = compute_metrics(exe_data[i], itk_data[i])
        results.append({
            "Projection": i,
            "MSE": f"{mse:.6f}",
            "RMSE": f"{rmse:.6f}",
            "CosineSimilarity": f"{c_sim:.6f}"
        })
        if i % 10 == 0:
            print(f"  Frame {i}/{num_proj}: RMSE={rmse:.4f}, CosSim={c_sim:.4f}")

    # Write CSV
    keys = results[0].keys()
    with open(csv_file, 'w', newline='') as f:
        dict_writer = csv.DictWriter(f, keys)
        dict_writer.writeheader()
        dict_writer.writerows(results)
    
    print(f"\nVerification complete. Results saved to {csv_file}")
    
    # Summary statistics
    all_rmse = [float(r["RMSE"]) for r in results]
    print(f"Average RMSE: {np.mean(all_rmse):.6f}")
    print(f"Average Cosine Similarity: {np.mean([float(r['CosineSimilarity']) for r in results]):.6f}")

if __name__ == "__main__":
    main()
