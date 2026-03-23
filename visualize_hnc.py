import os
import struct
import numpy as np
import matplotlib.pyplot as plt
import sys

def visualize_projection(filepath):
    """
    Reads a Varian .hnc or compressed .bin projection file and plots it.
    """
    if not os.path.exists(filepath):
        print(f"Error: File not found: {filepath}")
        return

    ext = os.path.splitext(filepath)[1].lower()
    
    try:
        angle = None
        if ext == '.bin':
            # Compressed projections from compressProj.m:
            # - No header
            # - Float32 (single)
            # - Resized to 128x128
            size_x, size_y = 128, 128
            with open(filepath, 'rb') as f:
                data = np.fromfile(f, dtype=np.float32)
            
            # compressProj.m does: P = reshape(P,[128*128,1]);
            # Image is already flattened, just reshape back.
            if len(data) == size_x * size_y:
                image = data.reshape((size_y, size_x))
            else:
                print(f"Warning: .bin data size ({len(data)}) does not match 128x128")
                return
        else:
            # Standard .hnc logic
            with open(filepath, 'rb') as f:
                header = f.read(512)
                size_x = struct.unpack('<I', header[120:124])[0]
                size_y = struct.unpack('<I', header[124:128])[0]
                
                try:
                    angle_offset = 128 + 8 + 16 + 4 + 4 + 4 + 4 + 8*31 
                    angle = struct.unpack('<d', header[angle_offset:angle_offset+8])[0]
                except:
                    pass

                data = np.fromfile(f, dtype=np.uint16)
                if len(data) == size_x * size_y:
                    image = data.reshape((size_y, size_x))
                else:
                    image = data[:size_x*size_y].reshape((size_y, size_x))

        print(f"File: {os.path.basename(filepath)}")
        print(f"Type: {ext}")
        print(f"Dimensions: {size_x} x {size_y}")
        if angle is not None:
            print(f"Gantry Angle: {angle:.2f} deg")

        # Plotting
        plt.figure(figsize=(10, 8))
        # bin files are log-attenuation (typically inverted/processed), 
        # hnc files are raw counts.
        plt.imshow(image, cmap='gray', origin='lower')
        plt.colorbar(label='Intensity')
        title = f"Projection: {os.path.basename(filepath)}"
        if angle is not None:
            title += f" ({angle:.2f}°)"
        plt.title(title)
        plt.axis('off')
        plt.tight_layout()
        plt.show()

    except Exception as e:
        print(f"Error reading projection file: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        visualize_projection(sys.argv[1])
    else:
        print("Usage: python visualize_hnc.py <filepath.hnc or filepath.bin>")
