import sys
import os

# Add the root directory to the Python path to allow absolute imports
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from gui.app import main

if __name__ == "__main__":
    main()
