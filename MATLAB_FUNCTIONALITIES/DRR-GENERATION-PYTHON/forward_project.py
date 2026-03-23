import itk
import numpy as np
import cv2
import imageio
import os
from pathlib import Path

def generate_drrs():
    print("Loading CT volume and geometry...")
    # 1. Load CT Volume (ensure float32)
    vol_path = "CT_01.mha"
    geometry_path = "RTKGeometry_360.xml"
    
    if not os.path.exists(vol_path) or not os.path.exists(geometry_path):
        print(f"Error: Missing {vol_path} or {geometry_path}")
        return

    itk_image = itk.imread(vol_path, itk.F)
    
    # 2. Load Geometry
    rtk = itk.RTK
    geometry_reader = rtk.ThreeDCircularProjectionGeometryXMLFileReader.New()
    geometry_reader.SetFilename(geometry_path)
    geometry_reader.GenerateOutputInformation()
    geometry = geometry_reader.GetOutputObject()
    
    num_projections = len(geometry.GetGantryAngles())
    print(f"Geometry loaded: {num_projections} projections.")

    # 3. Create constant image for output stack
    # Match MATLAB specs: 1024x768, spacing 0.388
    # RTK order is X, Y, Z (where Z is frames in circular geometry)
    constant_source = rtk.ConstantImageSource[itk.Image[itk.F, 3]].New()
    constant_source.SetOrigin([-200, -150, 0]) # approximate center
    constant_source.SetSpacing([0.388, 0.388, 1.0])
    constant_source.SetSize([1024, 768, num_projections])
    constant_source.SetConstant(0.0)
    
    # 4. Set up Joseph Forward Projector
    print("Starting forward projection...")
    projector = rtk.JosephForwardProjectionImageFilter[itk.Image[itk.F, 3], itk.Image[itk.F, 3]].New()
    projector.SetInput(0, constant_source.GetOutput())
    projector.SetInput(1, itk_image)
    projector.SetGeometry(geometry)
    projector.Update()
    
    # 5. Extract results and scale for visualization
    # To have bone as white and air as black, we use the attenuation values directly
    drr_stack = itk.GetArrayFromImage(projector.GetOutput())
    print(f"Projection stack shape: {drr_stack.shape}")
    
    # Normalize drr_stack (attenuation) to 0-255 for standard visualization
    # Lower values (air) -> Black, Higher values (bone) -> White
    drr_min = np.min(drr_stack)
    drr_max = np.max(drr_stack)
    print(f"Attenuation range: {drr_min:.2f} to {drr_max:.2f}")
    
    # Map to 0-255
    # We clip to handle potential outliers or noise if necessary, 
    # but here we'll just scale linearly
    intensity_stack_8bit = ((drr_stack - drr_min) / (drr_max - drr_min) * 255).astype(np.uint8)
    
    # 6. Save frames for video and output
    print("Saving inverted projections and generating video...")
    output_frames = []
    fps = 15
    
    for i in range(num_projections):
        frame_norm = intensity_stack_8bit[i] # Shape (768, 1024)
        output_frames.append(frame_norm)
        
        # Save every 10th frame as image for verification
        if i % 10 == 0:
            cv2.imwrite(f"Proj_{i:03d}.png", frame_norm)
            
    # 7. Generate MP4
    height, width = output_frames[0].shape
    fourcc = cv2.VideoWriter_fourcc(*'mp4v') # use mp4v for high compatibility
    out_video = cv2.VideoWriter('DRR_360_Simulation.mp4', fourcc, fps, (width, height), False)
    
    for frame in output_frames:
        out_video.write(frame)
    out_video.release()
    
    print("Done! Video saved as DRR_360_Simulation.mp4")

if __name__ == "__main__":
    generate_drrs()
