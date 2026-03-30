"""Learn-GUI-Python entry point."""
import os
import sys

_root = os.path.dirname(os.path.abspath(__file__))
if _root not in sys.path:
    sys.path.insert(0, _root)

# macOS + conda: Qt often fails with "Could not find the Qt platform plugin cocoa in """
# Set plugin paths before any PyQt6 import (must run before gui.app loads QtWidgets).
if sys.platform == "darwin":
    if not os.environ.get("QT_QPA_PLATFORM_PLUGIN_PATH", "").strip():
        _platforms = None
        _qt6 = None
        try:
            import PyQt6

            _qt6 = os.path.join(os.path.dirname(PyQt6.__file__), "Qt6", "plugins")
            _cand = os.path.join(_qt6, "platforms")
            if os.path.isdir(_cand):
                _platforms = _cand
        except ImportError:
            pass
        if _platforms is None:
            import site

            for base in site.getsitepackages() + ([site.getusersitepackages()] if site.getusersitepackages() else []):
                _cand = os.path.join(base, "PyQt6", "Qt6", "plugins", "platforms")
                if os.path.isdir(_cand):
                    _platforms = _cand
                    _qt6 = os.path.dirname(_cand)
                    break
        if _platforms:
            os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = _platforms
        if _qt6 and os.path.isdir(_qt6) and not os.environ.get("QT_PLUGIN_PATH", "").strip():
            os.environ["QT_PLUGIN_PATH"] = _qt6

from gui.app import main

if __name__ == "__main__":
    main()
