import itk
import os
import pydicom
import numpy as np
from pathlib import Path
from collections import defaultdict

def _dicom_description_to_phase_bin(descript: str) -> int:
    """
    Replicates MATLAB's dicomDscrptToPhaseBin.m logic.
    Maps "Gated 50%" to 6, "Gated 0%" to 1, etc.
    """
    if not descript or 'Gated' not in descript:
        return 0
    try:
        # Example: "Gated 50%" -> parts=["", " 50%"]
        after_gated = descript.split('Gated')[1]
        # " 50%" -> "50"
        num_str = after_gated.split('%')[0].strip()
        if ',' in num_str:
            num_str = num_str.split(',')[-1].strip()
        
        val = float(num_str)
        return int(round(val / 10.0) + 1)
    except (ValueError, IndexError):
        return 0

def convert_dicom_to_mha(dicom_directory, output_base_path):
    """
    Converts DICOM slices in a directory to one or more .mha files (one per phase).
    Matches MATLAB's DicomToMha_VALKIM.m logic:
    - Groups by Phase (SeriesDescription)
    - Scales HU to Attenuation coefficients ((HU+1000) * 0.013 / 1000)
    - Correct Z-sorting via ITK GDCMSeriesFileNames
    """
    dicom_path = Path(dicom_directory)
    if not dicom_path.exists():
        print(f"Error: Directory {dicom_directory} does not exist.")
        return []

    output_dir = Path(output_base_path).parent
    output_dir.mkdir(parents=True, exist_ok=True)

    # 1. Group files by Series and Phase
    print(f"Scanning {dicom_directory} for CT series...")
    groups = defaultdict(list)
    
    for f in dicom_path.rglob("*"):
        if not f.is_file(): continue
        try:
            # Quick check for DICOM magic bytes
            with open(f, 'rb') as f_binary:
                f_binary.seek(128)
                if f_binary.read(4) != b'DICM': continue
            
            ds = pydicom.dcmread(str(f), stop_before_pixels=True)
            if getattr(ds, 'Modality', '') != 'CT': continue
            
            desc = str(getattr(ds, 'SeriesDescription', ''))
            phase = _dicom_description_to_phase_bin(desc)
            # Default to 1 if no phase detected but it's a CT
            if phase == 0: phase = 1
            
            groups[phase].append(f)
        except Exception:
            continue

    if not groups:
        print("No CT DICOM files found.")
        return []

    generated_files = []
    series_info_lines = []
    water_att = 0.013

    # Setup ITK components
    ImageType = itk.Image[itk.F, 3]
    reader = itk.ImageSeriesReader[ImageType].New()
    dicom_io = itk.GDCMImageIO.New()
    reader.SetImageIO(dicom_io)
    names_gen = itk.GDCMSeriesFileNames.New()
    names_gen.SetDirectory(str(dicom_directory))

    # 2. Process each phase group
    for phase_bin in sorted(groups.keys()):
        files_in_phase = [str(f) for f in groups[phase_bin]]
        phase_label = f"Phase {phase_bin:02d}"
        output_filename = f"CT_{phase_bin:02d}.mha"
        target_path = output_dir / output_filename
        
        print(f"\nProcessing {phase_label} ({len(files_in_phase)} slices)...")
        
        # Get all series UIDs discovered by ITK
        all_series = names_gen.GetSeriesUIDs()
        if not all_series:
            print(f"  Warning: ITK could not find UIDs for {phase_label}")
            continue
            
        # Find which ITK series matches our phase files
        chosen_files = []
        for uid in all_series:
            itk_names = names_gen.GetFileNames(uid)
            # Check overlap
            overlap = [n for n in itk_names if Path(n) in groups[phase_bin]]
            if len(overlap) > len(chosen_files):
                chosen_files = overlap
        
        if not chosen_files:
            print(f"  Warning: No matching files found for {phase_label}")
            continue

        print(f"  Converting {len(chosen_files)} sorted slices...")
        reader.SetFileNames(chosen_files)
        
        try:
            reader.Update()
            image = reader.GetOutput()
            
            # 3. Intensity Scaling: (HU + 1000) * 0.013 / 1000
            arr = itk.array_from_image(image)
            print(f"  Scaling intensities (HU -> Linear Attenuation)...")
            arr = (arr + 1000.0) * water_att / 1000.0
            
            scaled_image = itk.image_view_from_array(arr.astype(np.float32))
            scaled_image.CopyInformation(image)
            
            print(f"  Saving to {target_path}...")
            itk.imwrite(scaled_image, str(target_path))
            generated_files.append(str(target_path))
            
            # Metadata for SeriesInfo.txt
            try:
                ds = pydicom.dcmread(chosen_files[0], stop_before_pixels=True)
                desc = ds.get("SeriesDescription", "CT series")
                series_info_lines.append(f"CT_{phase_bin:02d}: {desc}")
            except:
                series_info_lines.append(f"CT_{phase_bin:02d}: CT series")
                
            print(f"  Done.")
            
        except Exception as e:
            print(f"  Error processing {phase_label}: {e}")

    # 4. Write SeriesInfo.txt for GUI monitor parity
    if series_info_lines:
        try:
            with open(output_dir / "SeriesInfo.txt", "w", encoding="utf-8") as f:
                f.write("\n".join(series_info_lines))
        except Exception as e:
            print(f"Warning: Could not create SeriesInfo.txt: {e}")

    return generated_files

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 2:
        convert_dicom_to_mha(sys.argv[1], sys.argv[2])
    else:
        print("Usage: convert.py <dicom_dir> <output_mha_sample_path>")
