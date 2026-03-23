import sys
from pathlib import Path

# Add src to path
src_path = Path(r"C:\Users\aseb0376\OneDrive - The University of Sydney (Staff)\Documents\LEARN-GUI-main\LEARN-GUI-main\src")
if str(src_path) not in sys.path:
    sys.path.append(str(src_path))

from voxelmap_gui.step_managers import get_projections_per_ct

count = get_projections_per_ct()
print(f"Detected projection count: {count}")
