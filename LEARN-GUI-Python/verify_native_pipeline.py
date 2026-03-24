# verify_native_pipeline.py - STANDALONE PIPELINE VERIFIER
import sys
import os
import time
import tempfile
import traceback
from pathlib import Path
from PyQt6.QtCore import pyqtSlot, pyqtSignal, QObject, QCoreApplication, QTimer

# Ensure current directory is in path
sys.path.append(os.getcwd())

# Global exception handler
def global_exception_handler(exctype, value, tb):
    print("FATAL UNHANDLED EXCEPTION:")
    traceback.print_exception(exctype, value, tb)
    sys.exit(1)

sys.excepthook = global_exception_handler

from gui.controller import MainController
import gui.backend as backend
from gui.audit import AuditLog

def log_print(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}")
    sys.stdout.flush()

# Better Mock QMessageBox
class MockQMessageBox:
    class Icon:
        Critical = 1
        Information = 2
        Warning = 3
        Question = 4
    class StandardButton:
        Yes = 1
        No = 0
        Ok = 1
        Apply = 2
        Cancel = 3
    
    def __init__(self, *args, **kwargs): pass
    def setIcon(self, *args): pass
    def setWindowTitle(self, *args): pass
    def setText(self, *args): pass
    def setInformativeText(self, *args): pass
    def setStandardButtons(self, *args): pass
    def setDefaultButton(self, *args): pass
    def exec(self): return 1
    @staticmethod
    def question(*args, **kwargs): return 1
    @staticmethod
    def information(*args, **kwargs): return 1
    @staticmethod
    def critical(*args, **kwargs): 
        log_print(f"CRITICAL UI ERROR: {args}")
        return 1

class MockCaseDock(QObject):
    data_root_changed = pyqtSignal(str)
    def __init__(self, parent=None):
        super().__init__(parent)
        self.trial = "VALKIM"
        self.centre = "RNSH"
    def current_trial(self): return self.trial
    def current_centre(self): return self.centre

class MockTab(QObject):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.btn_open_log = MockButton()
        self.ct_quad = MockViewer()
        self.drr_viewer = MockViewer()
        self.dvf_viewer = MockViewer()
        self.step_selection = [True] * 10 
        self.bar = MockProgressBar() 
        self.bar_info = MockLabel()
        self.lbl_mha_detail = MockLabel()
        self.lbl = MockLabel()
        
    def activate_session(self): log_print("TAB: activate_session")
    def set_busy(self, b): log_print(f"TAB: set_busy({b})")
    def reset_all_steps(self, from_idx=0): log_print(f"TAB: reset_all_steps({from_idx})")
    def reset_session(self): log_print("TAB: reset_session")
    def reset_viewers(self): log_print("TAB: reset_viewers")
    def is_step_selected(self, idx): return self.step_selection[idx]
    def set_step_status(self, idx, status, pct=None):
        log_print(f"[STEP {idx}] {status} ({pct if pct else '0'}%)")
    def update_step_detail(self, idx, text):
        log_print(f"[DETAIL {idx}] {text}")
    def get_selected_steps(self): return ["Step"] * 10
    def set_copy_counts(self, c, t): log_print(f"TAB: set_copy_counts({c}, {t})")
    def clear_bar_info(self): pass
    def clear_mha_subcounts(self): pass
    def set_mha_subcounts(self, *args): log_print(f"TAB: set_mha_subcounts{args}")
    def set_active_step(self, idx): log_print(f"TAB: set_active_step({idx})")
    def show_preparing(self, label): log_print(f"TAB: show_preparing({label})")
    def switch_viewer(self, name): log_print(f"TAB: switch_viewer({name})")
    def blank_all_viewers(self): log_print("TAB: blank_all_viewers")
    def set_step_completed(self, idx, b): log_print(f"TAB: set_step_completed({idx}, {b})")
    def set_step_error(self, idx, b): log_print(f"TAB: set_step_error({idx}, {b})")
    def get_current_step_availability(self): return {}

class MockProgressBar:
    def setValue(self, v): pass
    def setRange(self, a, b): pass
    def setStyleSheet(self, s): pass

class MockLabel:
    def setText(self, t): pass
    def clear(self): pass
    def setVisible(self, b): pass
    def setAlignment(self, a): pass
    def setStyleSheet(self, s): pass
    def setMinimumWidth(self, w): pass
    def setVisible(self, v): pass

class MockButton:
    def setEnabled(self, b): pass

class MockViewer:
    def __init__(self):
        self.info_panel = MockInfoPanel() 
    def set_axial_pixmap(self, p): pass
    def set_info_dict(self, d): pass
    def set_drr_pixmap(self, p, angle): pass
    def set_viewer_mode(self, mode): pass
    def clear(self): pass
    def repaint(self): pass
    def add_frame(self, *args): pass
    def set_image(self, *args): pass
    def set_text(self, *args): pass

class MockInfoPanel:
    def set_preview_mode(self, b): pass

class MockWin(QObject):
    def __init__(self):
        super().__init__()
        self.tab2d = MockTab(self)
        self.case_dock = MockCaseDock(self)
    def clear_viewer_tabs(self): log_print("WIN: clear_viewer_tabs")

def verify():
    app = QCoreApplication(sys.argv)
    
    # Monkeypatch API call
    def mock_resolve(trial, centre, ids):
        log_print(f"Mocking resolve_rt_ct_pres for {ids}")
        return ["/VALKIM/RNSH/PlanningFiles/Patient01/"]
    backend.resolve_rt_ct_pres = mock_resolve
    
    # Monkeypatch QMessageBox in controller.py module
    import gui.controller as controller_mod
    controller_mod.QMessageBox = MockQMessageBox
    
    # Setup temporary environment
    temp_dir = Path(tempfile.mkdtemp(prefix="voxelmap_test_"))
    log_print(f"Using TEMPORARY WORK_ROOT: {temp_dir}")
    
    audit = AuditLog(temp_dir / "audit.log")
    win = MockWin()
    
    controller = MainController(win, audit, temp_dir)
    controller.selected_ids = ["VALKIM_01"]
    
    # Redirect controller logging
    def verbose_log(msg, level="INFO"):
        log_print(f"CONTROLLER LOG [{level}]: {msg}")
    controller.log = verbose_log

    log_print(f"--- STARTING VERIFICATION RUN ---")
    
    # Start the pipeline
    try:
        controller.start_run_pipeline()
    except Exception as e:
        log_print(f"FATAL: Pipeline failed to start: {e}")
        traceback.print_exception(type(e), e, e.__traceback__)
        return

    # Monitor
    start_time = time.time()
    timeout = 60 
    
    log_print(f"Monitoring for {timeout} seconds...")
    last_step = None

    try:
        while time.time() - start_time < timeout:
            app.processEvents()
            
            current_manager = controller._current_step_manager
            if current_manager:
                if current_manager != last_step:
                    log_print(f"--- NOW RUNNING STEP: {current_manager.__class__.__name__} ---")
                    last_step = current_manager
            
            if not controller._step_queue and controller._current_step_manager is None:
                log_print("--- VERIFICATION COMPLETE: ALL QUEUED STEPS FINISHED ---")
                break
            
            time.sleep(0.5)
    except Exception as e:
        log_print(f"EVENT LOOP FATAL: {e}")
        traceback.print_exception(type(e), e, e.__traceback__)

    log_print("--- PIPELINE STATE AT END ---")
    log_print(f"Step Queue: {controller._step_queue}")
    log_print(f"Current Manager: {controller._current_step_manager}")
    
    app.quit()

if __name__ == "__main__":
    verify()
