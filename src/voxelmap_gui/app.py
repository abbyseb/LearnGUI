# app.py
import sys
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import qInstallMessageHandler
from .mainwindow import MainWindow
import SimpleITK as sitk

def _qt_msg_handler(msg_type, context, message):
    """Filter out harmless startup/resize painter spam; keep other messages."""
    if message.startswith("QPainter::"):
        return
    sys.stderr.write(message + "\n")


def main():

    sitk.ProcessObject_SetGlobalWarningDisplay(False)
    # Install message handler before QApplication is created
    qInstallMessageHandler(_qt_msg_handler)

    app = QApplication(sys.argv)
    win = MainWindow()  # MainWindow sets its own maximized state

    # Log any uncaught exception and ensure MATLAB is stopped.
    def _exhook(exctype, value, tb):
        try:
            reason = f"{exctype.__name__}: {value}"
            if hasattr(win, 'controller') and hasattr(win.controller, 'emergency_shutdown'):
                win.controller.emergency_shutdown(reason)
        except Exception:
            pass
        # still print to console for development visibility
        sys.__excepthook__(exctype, value, tb)

    sys.excepthook = _exhook

    win.show()  # window state handled in MainWindow.__init__
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
