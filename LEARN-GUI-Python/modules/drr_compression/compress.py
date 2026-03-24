import numpy as np
import os
import glob
import struct
from scipy.ndimage import zoom

def read_hnc(filename):
    """
    Reads a Varian .hnc or .hnd file.
    """
    with open(filename, 'rb') as f:
        header_data = f.read(512)
        # uiSizeX is at offset 116 (32+4+4+4+8+8+16+4+16+4+16+4)
        # Wait, let's look at the MATLAB code offsets again:
        # 32 (bFileType) 
        # 4 (uiFileLength)
        # 4 (bChecksumSpec)
        # 4 (uiCheckSum)
        # 8 (bCreationDate)
        # 8 (bCreationTime)
        # 16 (bPatientID)
        # 4 (uiPatientSer)
        # 16 (bSeriesID)
        # 4 (uiSeriesSer)
        # 16 (bSliceID)
        # 4 (uiSliceSer)
        # 4 (uiSizeX) -> Target!
        # 4 (uiSizeY) -> Target!
        
        # Total offset to uiSizeX = 32+4+4+4+8+8+16+4+16+4+16+4 = 120 bytes
        # Let's use struct.unpack starting at 120
        uiSizeX = struct.unpack('<I', header_data[120:124])[0]
        uiSizeY = struct.unpack('<I', header_data[124:128])[0]
        
        # Image data starts after 512 bytes
        f.seek(512)
        n_pixels = uiSizeX * uiSizeY
        image_data = f.read(n_pixels * 2) # uint16
        M = np.frombuffer(image_data, dtype=np.uint16).reshape((uiSizeY, uiSizeX))
        
        return M, uiSizeX, uiSizeY

def compress_projection(input_path, output_path):
    """
    Compresses a projection matching the MATLAB logic.
    """
    P, nx, ny = read_hnc(input_path)
    
    # MATLAB: P = single(P');
    P = P.T.astype(np.float32)
    
    # MATLAB: P = log(65536 ./ (P + 1));
    P = np.log(65536.0 / (P + 1.0))
    
    # MATLAB: P = P(1:size(P,1)/128:end, 1:size(P,2)/128:end);
    # This is effectively downsampling to 128x128 if the original is a multiple of 128.
    # We can use zoom or slicing.
    zoom_x = 128.0 / P.shape[0]
    zoom_y = 128.0 / P.shape[1]
    P_resampled = zoom(P, (zoom_x, zoom_y), order=1) # Bilinear interpolation
    
    # MATLAB: P = reshape(P,[128*128,1]);
    P_flattened = P_resampled.flatten()
    
    # Save as binary file (32-bit float)
    with open(output_path, 'wb') as f:
        f.write(P_flattened.tobytes())
    
    print(f"Compressed {input_path} to {output_path}")

def process_directory(directory):
    files = glob.glob(os.path.join(directory, "*.hnc")) + glob.glob(os.path.join(directory, "*.hnd"))
    for f in files:
        out_name = os.path.splitext(f)[0] + ".bin"
        compress_projection(f, out_name)

if __name__ == "__main__":
    process_directory(".")
