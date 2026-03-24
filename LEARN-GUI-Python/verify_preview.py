import pydicom
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import os

def get_preview_data(dcm_path):
    try:
        ds = pydicom.dcmread(dcm_path)
        img = ds.pixel_array
        
        # Rescale if needed
        slope = getattr(ds, "RescaleSlope", 1)
        intercept = getattr(ds, "RescaleIntercept", 0)
        img = img * slope + intercept
        
        # Windowing
        lw = getattr(ds, "WindowWidth", 400)
        ll = getattr(ds, "WindowCenter", 40)
        if isinstance(lw, pydicom.multival.MultiValue): lw = lw[0]
        if isinstance(ll, pydicom.multival.MultiValue): ll = ll[0]
        
        min_val = ll - lw/2
        max_val = ll + lw/2
        img = np.clip(img, min_val, max_val)
        img = (img - min_val) / (max_val - min_val)
        
        info = {
            "PatientID": getattr(ds, "PatientID", "N/A"),
            "SliceLocation": getattr(ds, "SliceLocation", "N/A"),
            "InstanceNumber": getattr(ds, "InstanceNumber", "N/A"),
            "Filename": Path(dcm_path).name
        }
        return img, info
    except Exception as e:
        print(f"Error reading {dcm_path}: {e}")
        return None, None

def create_preview_plot(folder_path, output_path):
    files = [f for f in Path(folder_path).glob("*.dcm")]
    files.sort(key=lambda x: x.name)
    
    if not files:
        print("No DICOM files found.")
        return

    # Take 9 slices spread across the series
    indices = np.linspace(0, len(files)-1, 9, dtype=int)
    
    fig, axes = plt.subplots(3, 3, figsize=(15, 15))
    fig.suptitle(f"DICOM Preview Verification - Folder: {Path(folder_path).parent.name}", fontsize=16)
    
    for i, idx in enumerate(indices):
        ax = axes[i // 3, i % 3]
        img, info = get_preview_data(files[idx])
        if img is not None:
            ax.imshow(img, cmap="gray")
            ax.set_title(f"Slice {info['InstanceNumber']}\nPos: {info['SliceLocation']}")
        ax.axis("off")
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(output_path)
    print(f"Plot saved to {output_path}")

if __name__ == "__main__":
    sample_dir = r"C:\Users\aseb0376\OneDrive - The University of Sydney (Staff)\Documents\LEARN-GUI-main\LEARN-GUI-main\LEARN-GUI-Python\Runs\VALKIM_01_Run_20260324_101600\Patient01\train"
    out_img = r"C:\Users\aseb0376\.gemini\antigravity\brain\368272b5-b35e-446e-b108-621698beb8b7\preview_verification.png"
    create_preview_plot(sample_dir, out_img)
