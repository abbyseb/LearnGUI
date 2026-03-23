# controller.py - REFACTORED WITH STEP MANAGERS
"""
Simplified controller using self-contained step managers.
Each step handles its own progress and viewer - works identically for runs/reruns.
"""
from __future__ import annotations
from pathlib import Path
from typing import List, Optional
import traceback
import re # Import re module

from PyQt6.QtWidgets import QMessageBox
from PyQt6.QtCore import QObject, Qt, pyqtSignal, QTimer, QUrl
from PyQt6.QtGui import QDesktopServices

from .audit import AuditLog
from .backend import resolve_rt_ct_pres, BASE_API, BASE_DIR
from .workers import MatlabJob
from .run_preparation import (
    patient_id_from_rt, centre_from_rt, selection_line,
    build_train_pairs_for_run, count_dicoms_recursive,
    validate_rerun_prerequisites, purge_step_outputs_from_train,
    get_step_output_info, get_step_prerequisite_info,
    get_downstream_steps, check_step_prerequisites_exist
)
from .run_folder_manager import create_run_structure, get_run_log_file, get_train_dir
from .step_managers import create_step_manager
from .viewer_monitors import create_viewer_monitor


# Step configuration
STEP_ORDER = ["COPY", "DICOM2MHA", "DOWNSAMPLE", "DRR", "COMPRESS", "DVF2D", "DVF3D_LOW", "DVF3D_FULL", "KV_PREPROCESS"]

STEP_DISPLAY_NAMES = {
    "COPY": "Copy training files",
    "DICOM2MHA": "DICOM → MHA",
    "DOWNSAMPLE": "Downsample volumes",
    "DRR": "Generate DRRs",
    "COMPRESS": "Compress DRRs",
    "DVF2D": "2D DVFs",
    "DVF3D_LOW": "3D DVFs (downsampled)",
    "DVF3D_FULL": "3D DVFs (full resolution)",
    "KV_PREPROCESS": "Finalise & Process kV",
}

STEP_UI_INDEX = {step: idx for idx, step in enumerate(STEP_ORDER)}

VIEWER_STEP_MAP = {
    "DICOM2MHA": "ct",
    "DRR": "drr",
    "DVF3D_LOW": "dvf",
    "DVF3D_FULL": "dvf",
    # Steps that show "preparing" or linger on previous viewer
    "DOWNSAMPLE": "preparing",
    "COMPRESS": "preparing",
    "DVF2D": "preparing",
    "KV_PREPROCESS": "preparing",
}

class MainController(QObject):
    """Simplified controller with step manager architecture."""
    
    pipeline_completed = pyqtSignal()
    step_viewer_completed = pyqtSignal(int, str, str)

    
    def __init__(self, window, audit: AuditLog, work_root: Path):
        super().__init__()
        self.win = window
        self.audit = audit
        self.work_root = Path(work_root)
        
        # Save session log for fallback when run log fails
        self._session_log_path = audit.logfile
        
        # Run tracking
        self.current_run_folder: Optional[Path] = None
        self.current_run_log: Optional[Path] = None
        self.selected_ids: List[str] = []
        self._last_rt_list: List[str] = []
        
        # Session state
        self._session_active = False
        self._is_rerun_mode = False
        
        # Step execution
        self._current_step_manager = None
        self._current_viewer_monitor = None
        self._active_step_name: Optional[str] = None
        self._train_dir: Optional[Path] = None
        
        # MATLAB job
        self._job: Optional[MatlabJob] = None
        
        # NEW: State variables for MATLAB log processing
        self._matlab_startup_buffer: List[str] = []
        self._matlab_has_started = False
        
        # UI state
        self._ui_frozen = False
        self._handling_error = False
        
        # Initialization
        if hasattr(self.audit, "start_session"):
            self.audit.start_session("Voxelmap GUI Session")
        self.log("GUI launched", level="DEBUG")
    
    # ========================================================================
    # LOGGING
    # ========================================================================
    
    def log(self, msg: str, level: str = "INFO"):
        if getattr(self, '_handling_error', False):
            return
            
        try:
            self.audit.add(msg, level=level)
        except Exception as e:
            from audit import LogFileDeletedError
            if isinstance(e, LogFileDeletedError):
                self._handling_error = True
                
                # Restore session log to write error
                self.audit.logfile = self._session_log_path
                try:
                    self.audit.add(f"CRITICAL: Run log deleted during execution: {self.current_run_log}", level="ERROR")
                except:
                    pass
                
                # Disable further writes to run log
                self.current_run_log = None
                self.audit.logfile = None
                
                self._finish_err("Log file deleted - check session log")
            else:
                print(f"Logging error: {e}")
    
    @property
    def run_log_path(self) -> Optional[Path]:
        if self.current_run_log:
            return self.current_run_log
        if self.current_run_folder:
            return get_run_log_file(self.current_run_folder)
        if isinstance(self.audit.logfile, Path):
            return self.audit.logfile
        return None
    
    # ========================================================================
    # SELECTION
    # ========================================================================
    
    def on_selection(self, ids: List[str]):
        self.selected_ids = list(ids or [])
    
    # ========================================================================
    # STEP ACTIVATION (CORE LOGIC)
    # ========================================================================
    
    def _activate_step(self, step_name: str, **kwargs):
        """
        Core method: Activate a step manager and viewer monitor.
        Works identically for normal runs and reruns.
        """
        self._stop_current_step()
        
        self._active_step_name = step_name
        self._update_ui_for_step_start(step_name)
        
        # --- FIX: Smart viewer switching for COPY ---
        if step_name == "COPY":
            self.log("Activating 'ct' viewer panel for COPY step (no monitor).", level="DEBUG")
            # Manually switch the UI panel without starting a monitor
            self.win.tab2d.switch_viewer('ct')
            # Ensure the viewer is cleared to show DICOM preview (single) mode
            self.win.tab2d.ct_quad.clear() 
        else:
            viewer_type = VIEWER_STEP_MAP.get(step_name)
            if viewer_type:
                # This will start the appropriate monitor (e.g., CTViewerMonitor for DICOM2MHA)
                self._switch_viewer(viewer_type)
        # --- END FIX ---

        self._current_step_manager = create_step_manager(step_name, parent=self, **kwargs)
        self._current_step_manager.progress_updated.connect(self._on_step_progress)
        self._current_step_manager.step_complete.connect(
            lambda: self._on_step_complete(step_name)
        )
        
        if hasattr(self._current_step_manager, 'progress_detail'):
            self._current_step_manager.progress_detail.connect(self._on_progress_detail)
        
        self._current_step_manager.start(self._train_dir)
    
    def _on_progress_detail(self, *args):
        """Update right-side progress details."""
        try:
            if self._active_step_name == "COPY" and len(args) == 2:
                copied, total = args
                if total == 0:
                    self.win.tab2d.bar_info.setText(f"{copied} / ?")
                else:
                    self.win.tab2d.bar_info.setText(f"{copied} / {total}")
            
            elif self._active_step_name == "DOWNSAMPLE" and len(args) == 2:
                created, total = args
                self.win.tab2d.lbl.setText(f"Downsampling volumes... ")
                self.win.tab2d.bar_info.setText(f"{created}/{total}")
            
            elif self._active_step_name == "DICOM2MHA" and len(args) == 4:
                ct_done, ct_total, struct_done, struct_total = args
                ct_total_str = "?" if ct_total == -1 else str(ct_total)
                self.win.tab2d.bar_info.setText(f"CT: {ct_done}/{ct_total_str} | Struct: {struct_done}/{struct_total}")
            
            elif self._active_step_name == "DVF3D_LOW" and len(args) == 2:
                created, total = args
                self.win.tab2d.lbl.setText(f"Generating 3D DVFs (downsampled)... ({created}/{total})")
                self.win.tab2d.bar_info.setText(f"{created}/{total}")

            elif self._active_step_name == "DVF3D_FULL" and len(args) == 2:
                created, total = args
                self.win.tab2d.lbl.setText(f"Generating 3D DVFs (full res)... ({created}/{total})")
                self.win.tab2d.bar_info.setText(f"{created}/{total}")
            
            elif self._active_step_name == "DRR" and len(args) == 1:
                self.win.tab2d.bar_info.setText(args[0])
            
            elif self._active_step_name == "KV_PREPROCESS" and len(args) == 2:
                step_name, detail = args
                self.win.tab2d.lbl.setText(f"Finalising: {step_name}...")
                self.win.tab2d.bar_info.setText(detail)

        except Exception as e:
            print(f"Error updating progress detail: {e}")

    def _stop_current_step(self):
        """Clean up current step manager and viewer monitor."""
        if self._current_step_manager:
            self._current_step_manager.stop()
            self._current_step_manager.deleteLater()
            self._current_step_manager = None
        
        if self._current_viewer_monitor:
            self._current_viewer_monitor.stop()
            self._current_viewer_monitor.deleteLater()
            self._current_viewer_monitor = None
    
    def _switch_viewer(self, viewer_type: str):
        """Activate appropriate viewer and its monitor."""
        if self._current_viewer_monitor:
            self._current_viewer_monitor.stop()
            self._current_viewer_monitor.deleteLater()
            self._current_viewer_monitor = None
        
        if viewer_type == 'ct':
            self.win.tab2d.switch_viewer('ct')
            viewer_widget = self.win.tab2d.ct_quad
        elif viewer_type == 'drr':
            self.win.tab2d.switch_viewer('drr')
            viewer_widget = self.win.tab2d.drr_viewer
        elif viewer_type == 'dvf':
            self.win.tab2d.switch_viewer('dvf')
            viewer_widget = self.win.tab2d.dvf_viewer
        elif viewer_type == 'preparing':
            self.win.tab2d.switch_viewer('preparing')
            return # No viewer widget or monitor is needed for this state
        else:
            return
        
        self._current_viewer_monitor = create_viewer_monitor(
            viewer_type, viewer_widget, parent=self
        )
        self._current_viewer_monitor.start(self._train_dir)
    
    def _on_step_progress(self, pct: int, status: str):
        if self._ui_frozen: return
        self.win.tab2d.bar.setRange(0, 100)
        self.win.tab2d.bar.setValue(pct)
        self.win.tab2d.lbl.setText(status)
    
    def _safe_transition(self, step_name: str):
        """Transition only if job is still running and not in rerun mode."""
        if self._job and self._job.isRunning() and not self._is_rerun_mode:
            self._transition_to_next_step(step_name)

    def _transition_to_next_step(self, completed_step: str):
        """Finds the next *selected* step in the sequence and activates it."""
        if self._is_rerun_mode: return
            
        try:
            current_idx = STEP_ORDER.index(completed_step)
            next_step_to_run = None
            for i in range(current_idx + 1, len(STEP_ORDER)):
                step_name = STEP_ORDER[i]
                ui_index = STEP_UI_INDEX.get(step_name, -1)
                
                if ui_index != -1 and self.win.tab2d.is_step_selected(ui_index):
                    next_step_to_run = step_name
                    break
            
            if next_step_to_run:
                # FIX: Add a blank line to the log for better spacing between steps.
                self.audit.add_raw("")
                self.log(f"Transitioning: {completed_step} -> {next_step_to_run}")
                
                # Prepare kwargs for the next step
                step_kwargs = {}
                if next_step_to_run == "COPY":
                    self.log("Calculating total DICOMs for copy progress...")
                    pairs = build_train_pairs_for_run(self._last_rt_list, self.current_run_folder, BASE_DIR)
                    total_dicoms = sum(count_dicoms_recursive(src) for src, _ in pairs)
                    step_kwargs['total_dicoms'] = total_dicoms
                    self.log(f"Found {total_dicoms} DICOM files.")
                
                # --- START FIX ---
                # Add kwargs for KVPreprocessStepManager
                elif next_step_to_run == "KV_PREPROCESS":
                    step_kwargs['rt_ct_pres_list'] = self._last_rt_list
                    step_kwargs['base_dir'] = BASE_DIR
                # --- END FIX ---
                
                self._activate_step(next_step_to_run, **step_kwargs)
            else:
                self.log("All selected pipeline steps complete")
                self.pipeline_completed.emit()
                
        except (ValueError, IndexError):
            self.log(f"Warning: Could not find next step after '{completed_step}'.", level="WARN")


    def _on_step_complete(self, step_name: str):
        """Step completed - mark UI, create tab if needed, and schedule transition."""
        if self._ui_frozen: return
        
        idx = STEP_UI_INDEX.get(step_name, -1)
        if idx >= 0:
            self.win.tab2d.set_step_completed(idx, True)

        if step_name in VIEWER_STEP_MAP:
            display_name = STEP_DISPLAY_NAMES.get(step_name, step_name)
            viewer_type = VIEWER_STEP_MAP[step_name]
            self.step_viewer_completed.emit(idx + 1, display_name, viewer_type)

        self._check_rerun_availability()
        
        # FIX: Delay the "Step complete" log message to allow MATLAB's final
        # output to appear first, ensuring correct chronological order.
        QTimer.singleShot(750, lambda: (
            self.log(f"Step complete: {step_name}"),
            self._safe_transition(step_name)
        ))

    def _update_ui_for_step_start(self, step_name: str):
        idx = STEP_UI_INDEX.get(step_name, -1)
        if idx >= 0: self.win.tab2d.set_active_step(idx)
        display = STEP_DISPLAY_NAMES.get(step_name, step_name)
        self.win.tab2d.bar.setRange(0, 100)
        self.win.tab2d.bar.setValue(0)
        self.win.tab2d.lbl.setText(f"{display}...")
    
    # ========================================================================
    # NORMAL PIPELINE RUN
    # ========================================================================
    
    def start_run_pipeline(self):
        """Execute full pipeline with step managers."""
        try:
            if not self.selected_ids:
                QMessageBox.information(self.win, "Selection required", "Please select at least one patient.")
                return
            
            selected_steps_ui = self.win.tab2d.get_selected_steps()
            if not self._confirm_pipeline_steps(selected_steps_ui): return
            
            self._reset_all_state()
            self._is_rerun_mode = False
            
            trial = self.win.case_dock.current_trial()
            centre = self.win.case_dock.current_centre()
            
            try:
                rt_list = resolve_rt_ct_pres(trial, centre, self.selected_ids)
            except Exception as e:
                self.log(f"Prescriptions API failure: {e}", level="ERROR")
                QMessageBox.critical(self.win, "API Error", f"Failed to resolve patient prescriptions:\n{e}")
                return
            
            if not rt_list:
                self.log("No rt_ct_pres resolved for selection", level="ERROR")
                QMessageBox.critical(self.win, "Resolution Error", "No patient prescriptions found for the current selection.")
                return
            
            self._last_rt_list = rt_list[:]
            patient_id = patient_id_from_rt(rt_list[0], trial)
            
            self.current_run_folder, self.current_run_log = create_run_structure(self.work_root, patient_id)
            self.audit.set_log_file(self.current_run_log, append=False)
            
            self.audit.start_run()
            self.log(f"Patient: {patient_id}")
            self.log(f"Run Folder: {self.current_run_folder}")

            self._session_active = True
            self.win.tab2d.activate_session()
            self.win.tab2d.btn_open_log.setEnabled(True)
            
            pairs = build_train_pairs_for_run(rt_list, self.current_run_folder, BASE_DIR)
            if pairs: self._train_dir = pairs[0][1]
            
            self.win.tab2d.set_busy(True)
            self.win.tab2d.reset_all_steps()
            
            # --- REFACTORED BLOCK ---
            # Get all skip flags based on UI checkboxes
            skip_copy = not self.win.tab2d.is_step_selected(0)
            skip_dicom2mha = not self.win.tab2d.is_step_selected(1)
            skip_downsample = not self.win.tab2d.is_step_selected(2)
            skip_drr = not self.win.tab2d.is_step_selected(3)
            skip_compress = not self.win.tab2d.is_step_selected(4)
            skip_2d_dvf = not self.win.tab2d.is_step_selected(5)
            do_3d_low = self.win.tab2d.is_step_selected(6)
            do_3d_full = self.win.tab2d.is_step_selected(7)
            skip_kv_preprocess = not self.win.tab2d.is_step_selected(8)
            
            # Find the first step the user actually selected
            first_step_to_run = None
            for i, step_name in enumerate(STEP_ORDER):
                if self.win.tab2d.is_step_selected(i):
                    first_step_to_run = step_name
                    break # Found the first step
            
            if not first_step_to_run:
                QMessageBox.information(self.win, "No Steps Selected", "Please select at least one pipeline step to run.")
                self._reset_all_state()
                self.win.tab2d.set_busy(False)
                return

            self.log(f"Starting pipeline run. First step: {first_step_to_run}")

            # Prepare kwargs for the first step (e.g., total_dicoms for COPY)
            step_kwargs = {}
            if first_step_to_run == "COPY":
                self.log("Calculating total DICOMs for copy progress...")
                total_dicoms = sum(count_dicoms_recursive(src) for src, _ in pairs)
                step_kwargs['total_dicoms'] = total_dicoms
                self.log(f"Found {total_dicoms} DICOM files.")
            
            # Add kwargs for KVPreprocessStepManager if it's the *first* step
            elif first_step_to_run == "KV_PREPROCESS":
                step_kwargs['rt_ct_pres_list'] = self._last_rt_list
                step_kwargs['base_dir'] = BASE_DIR
            
            # FIX: Reset MATLAB logging state before starting a new run
            self._matlab_startup_buffer.clear()
            self._matlab_has_started = False
            
            self._job = MatlabJob(
                rt_ct_pres_list=rt_list,
                skip_copy=skip_copy,
                skip_dicom2mha=skip_dicom2mha,
                skip_downsample=skip_downsample,
                skip_drr=skip_drr,
                skip_compress=skip_compress,
                include_2d_dvf=not skip_2d_dvf,
                do_3d_low=do_3d_low,
                do_3d_full=do_3d_full,
                skip_kv_preprocess=skip_kv_preprocess,
                work_root=self.current_run_folder,
                log_file=self.current_run_log,
                mode="Pipeline",
                start_at="",
            )
            self._job.finished_ok.connect(self._finish_ok)
            self._job.failed.connect(self._finish_err)
            self._job.finished.connect(lambda: self.win.tab2d.set_busy(False))
            self._job.matlab_line_received.connect(self._process_log_line)
            self._job.start()
            
            # Activate the first selected step
            self._activate_step(first_step_to_run, **step_kwargs)
            
        except Exception as e:
            error_details = traceback.format_exc()
            self.log(f"FATAL PRE-RUN ERROR: {e}\n{error_details}", level="CRITICAL")
            self.win.tab2d.set_busy(False) # FIX: Reset UI on pre-run error
            
            box = QMessageBox(self.win)
            box.setIcon(QMessageBox.Icon.Critical)
            box.setWindowTitle("Application Error")
            box.setText("An unexpected error occurred before the pipeline could start.")
            box.setInformativeText(f"<b>Error:</b> {e}")
            box.setDetailedText(error_details)
            
            log_button = None
            session_log_path = self.audit.logfile
            if session_log_path and session_log_path.exists():
                log_button = box.addButton("Open Session Log", QMessageBox.ButtonRole.ActionRole)

            box.addButton("Close", QMessageBox.ButtonRole.RejectRole)
            box.exec()

            if log_button and box.clickedButton() == log_button:
                QDesktopServices.openUrl(QUrl.fromLocalFile(str(session_log_path)))
    
    def _flush_matlab_startup_log(self):
        """Writes the buffered MATLAB startup messages to the log in a clean block."""
        if not self._matlab_startup_buffer:
            return
        
        header = "MATLAB Environment Setup:"
        details = "\n".join([f"  > {line}" for line in self._matlab_startup_buffer])
        self.audit.add_raw(f"\n{header}\n{details}\n")
        self._matlab_startup_buffer.clear()

    def _process_log_line(self, line: str):
        """
        Process MATLAB output lines with error handling.
        Uses state machine to separate startup from pipeline progress.
        """
        try:
            line = line.strip()
            if not line:
                return

            if not self._matlab_has_started:
                start_marker_found = ('Processing rt_ct_pres:' in line or 
                                      'Found patient folder for rerun:' in line)
                
                if start_marker_found:
                    self._matlab_has_started = True
                    self._flush_matlab_startup_log()
                    self.log(line, level="MATLAB")
                else:
                    self._matlab_startup_buffer.append(line)
            else:
                self.log(line, level="MATLAB")

            # Check for step completion marker from MATLAB
            # Format could be %STEP_COMPLETE:STEP% or %%STEP_COMPLETE:STEP%%
            if "STEP_COMPLETE:" in line:
                # Extract the identifier more robustly
                try:
                    # Look for the identifier between "STEP_COMPLETE:" and a percent or end of line
                    parts = line.split("STEP_COMPLETE:")
                    raw_id = parts[1].replace("%", "").strip().upper()
                    
                    if raw_id:
                        self.log(f"MATLAB signaled completion of: {raw_id}", level="INFO")
                        
                        # Map MATLAB step names to Python names
                        step_map = {
                            "DVF3DLOW": "DVF3D_LOW",
                            "DVF3DFULL": "DVF3D_FULL",
                            "KVPREPROCESS": "KV_PREPROCESS",
                            "DOWNSAMPLE": "DOWNSAMPLE",
                        }
                        python_step = step_map.get(raw_id, raw_id)
                        
                        # Match against current active step (case-insensitive)
                        current_active = str(self._active_step_name).upper()
                        
                        if python_step == current_active:
                            self.log(f"Marker matches active step. Forcing transition for {python_step}.", level="INFO")
                            
                            # Stop current manager to prevent dual completion
                            if self._current_step_manager:
                                self._current_step_manager.stop()
                            
                            self._on_step_complete(python_step)
                        else:
                            self.log(f"Marker '{python_step}' received but active step is '{current_active}'", level="DEBUG")
                except Exception as ex:
                    self.log(f"Error parsing completion marker: {ex}", level="WARN")
        
        except Exception as e:
            # Any error in log processing should halt the pipeline
            self._finish_err(f"Log processing error: {str(e)}")

    def open_run_folder(self):
        """Open the current run folder in the system's file explorer."""
        if self.current_run_folder and self.current_run_folder.exists():
            QDesktopServices.openUrl(QUrl.fromLocalFile(str(self.current_run_folder)))
        else:
            QMessageBox.information(
                self.win,
                "No Folder Available",
                "No run folder is currently available.\n\n"
                "Start a pipeline run first."
            )

    # ========================================================================
    # RERUN STEP
    # ========================================================================
    
    def start_rerun_step(self, step: str):
        if not self._session_active or not self.current_run_folder:
            QMessageBox.warning(self.win, "No Active Session", "Rerun is only available after starting a pipeline.")
            return
        
        availability = self.win.tab2d.get_current_step_availability()
        step_display = self.win.tab2d.rerun_step_value()
        
        if step_display in availability and not availability[step_display]["available"]:
            reason = availability[step_display]["reason"]
            QMessageBox.critical(self.win, "Cannot Rerun", f"<b>Step '{step_display}' unavailable:</b><br><br>{reason}")
            return
        
        train_dir = get_train_dir(self.current_run_folder)
        patient_root = train_dir.parent
        
        if not self._show_rerun_confirmation(step, train_dir, patient_root):
            self.log("Rerun cancelled by user")
            return
        
        self.audit.add_raw(f"\n--- RERUN: {step} ---")
        self._is_rerun_mode = True
        
        pairs = [(Path(), train_dir)]
        success, err_msg = validate_rerun_prerequisites(pairs, patient_root, step, self.audit)
        if not success:
            QMessageBox.critical(self.win, "Validation Failed", f"{err_msg}\n\nSee log for details.")
            return
        
        deleted_count = purge_step_outputs_from_train(train_dir, step, self.work_root, self.audit)
        self.log(f"Purged {deleted_count} output file(s) from train/ directory")
        
        self.win.tab2d.set_busy(True)

        self.win.tab2d.blank_all_viewers()
        rerun_step_index = STEP_UI_INDEX.get(step.upper(), 0)
        self.win.tab2d.reset_all_steps(from_index=rerun_step_index)

        self._train_dir = train_dir
        
        # FIX: Reset MATLAB logging state before starting a rerun
        self._matlab_startup_buffer.clear()
        self._matlab_has_started = False
        
        self._job = MatlabJob(
            rt_ct_pres_list=self._last_rt_list, # Pass rt_list for COPY/KV steps
            skip_copy=True, skip_dicom2mha=True, skip_downsample=True,
            skip_drr=True, skip_compress=True,
            include_2d_dvf=True, # These don't matter for rerun
            do_3d_low=False, do_3d_full=False,
            skip_kv_preprocess=True,
            work_root=self.current_run_folder,
            log_file=self.current_run_log,
            mode="RerunStep",
            start_at=step,
        )
        self._job.finished_ok.connect(self._finish_ok_rerun)
        self._job.failed.connect(self._finish_err)
        self._job.finished.connect(lambda: self.win.tab2d.set_busy(False))
        self._job.matlab_line_received.connect(self._process_log_line)
        self._job.start()
        
        step_kwargs = {}
        if step.upper() == "COPY":
            # --- FIX: Calculate total_dicoms from original source for rerun ---
            if not self._last_rt_list:
                self.log("Rerun COPY: Cannot find original rt_list to count DICOMs.", level="ERROR")
                step_kwargs['total_dicoms'] = 0
            else:
                # Re-build the source paths just for counting
                pairs_for_counting = build_train_pairs_for_run(
                    self._last_rt_list, 
                    self.current_run_folder, 
                    BASE_DIR
                )
                self.log("Rerun COPY: Counting DICOMs from original source...")
                total_dicoms = sum(count_dicoms_recursive(src) for src, _ in pairs_for_counting)
                step_kwargs['total_dicoms'] = total_dicoms
                self.log(f"Found {total_dicoms} DICOMs for copy progress.")
            # --- END FIX ---

        elif step.upper() == "KV_PREPROCESS":
            step_kwargs['rt_ct_pres_list'] = self._last_rt_list
            step_kwargs['base_dir'] = BASE_DIR
        
        self._activate_step(step.upper(), **step_kwargs)
    
    def _finish_ok_rerun(self, msg: str):
        self._stop_current_step()
        self._job = None
        self.log(msg)
        self.audit.add_raw("--- Rerun Complete ---")
        self._is_rerun_mode = False
        self.win.tab2d.lbl.setText("Rerun completed successfully")
        self.win.tab2d.bar.setValue(100)
        self._check_rerun_availability()
    
    def _show_rerun_confirmation(self, step: str, train_dir: Path, patient_root: Path) -> bool:
        output_info = get_step_output_info(step.upper())
        downstream = get_downstream_steps(step.upper())
        
        msg_parts = [f"<b>Rerun Step: {step.upper()}</b>", "", "<b>This will:</b>", f"• Delete and recreate: {output_info['description']}"]
        if downstream:
            msg_parts.extend(["", "<b>⚠ Downstream Steps Will Be Marked Stale:</b>"])
            msg_parts.extend([f"  • {s}" for s in downstream])
        msg_parts.extend(["", "<b>Continue?</b>"])
        
        box = QMessageBox(self.win)
        box.setIcon(QMessageBox.Icon.Warning)
        box.setWindowTitle(f"Rerun: {step.upper()}")
        box.setTextFormat(Qt.TextFormat.RichText)
        box.setText("<br>".join(msg_parts))
        rerun_btn = box.addButton("Rerun", QMessageBox.ButtonRole.YesRole)
        cancel_btn = box.addButton("Cancel", QMessageBox.ButtonRole.NoRole)
        box.setDefaultButton(cancel_btn)
        box.exec()
        return box.clickedButton() is rerun_btn
    
    # ========================================================================
    # RERUN AVAILABILITY
    # ========================================================================
    
    def _check_rerun_availability(self):
        if not self.current_run_folder: return
        train_dir = get_train_dir(self.current_run_folder)
        patient_root = train_dir.parent
        if not train_dir.exists(): return
        
        availability = {}
        steps_to_check = {
            "Copy training files": "COPY",
            "DICOM → MHA": "DICOM2MHA", 
            "Downsample volumes": "DOWNSAMPLE",
            "Generate DRRs": "DRR", 
            "Compress DRRs": "COMPRESS", 
            "2D DVFs": "DVF2D",
            "3D DVFs (Downsampled)": "DVF3D_LOW",
            "3D DVFs (Full Resolution)": "DVF3D_FULL",
            "Finalize & Process kV": "KVPREPROCESS",
        }
        for display_name, step_key in steps_to_check.items():
            can_run, reason = check_step_prerequisites_exist(train_dir, patient_root, step_key)
            availability[display_name] = {"available": can_run, "reason": reason}
        self.win.tab2d.update_rerun_step_availability(availability)
    
    # ========================================================================
    # COMPLETION & CLEANUP
    # ========================================================================
    
    def _finish_ok(self, msg: str):
        self._stop_current_step()
        self._job = None
        self.log(msg)
        self.audit.end_run("OK")
        self.win.tab2d.lbl.setText("Completed successfully")
        self.win.tab2d.bar.setValue(100)
        self.pipeline_completed.emit()
    
    def _on_pipeline_complete(self):
        """
        Called when the entire pipeline completes.
        The "Main" tab remains "Main". Viewer tabs have already been created.
        """
        # --- ENTIRE BODY OF FUNCTION DELETED ---
        # The logic to rename the main tab was flawed and caused duplicate names.
        # The main tab should always be "Main".
        self.log("Pipeline complete. Main tab remains active.", level="DEBUG")

    def _finish_err(self, err: str):
        """Pipeline error handler - kills MATLAB and shows error."""
        self._stop_current_step()
        
        # Kill MATLAB job if still running
        if self._job and self._job.isRunning():
            try:
                self._job.cancel()
                self._job.wait(2000)
            except Exception:
                pass
        self._job = None
        
        self.log(f"Pipeline halted. Reason: {err}", level="ERROR")
        if self.audit.has_open_run(): 
            self.audit.end_run("ERROR")
        
        self.win.tab2d.set_busy(False)
        
        # Different message if log was deleted
        if "Log file deleted" in err or "check session log" in err:
            self.win.tab2d.lbl.setText("Error: Run log deleted. Check session log.")
        else:
            self.win.tab2d.lbl.setText("Error Detected. Check log.")
        
        self.win.tab2d.bar.setRange(0, 100)
        self.win.tab2d.bar.setValue(100)
        self.win.tab2d.bar.setStyleSheet("QProgressBar::chunk { background-color: #e74c3c; }")

    def _reset_all_state(self):
        self._stop_current_step()
        self.win.tab2d.reset_all_steps()
        self.win.tab2d.reset_session()
        
        self.win.tab2d.bar.setValue(0)
        self.win.tab2d.reset_viewers()

        self.win.clear_viewer_tabs()
        self._ui_frozen = False
        self._session_active = False
    
    def _confirm_pipeline_steps(self, selected_steps: List[int]) -> bool:
        return True
    
    # ========================================================================
    # CANCELLATION
    # ========================================================================

    def _shutdown_pipeline(self):
        """Gracefully stops all running pipeline components."""
        self.log("Pipeline cancelled by user.", level="WARN")
        
        self._stop_current_step()
            
        if self._job: self._job.cancel()
            
        self.win.tab2d.set_busy(False)
        self.win.tab2d.lbl.setText("Cancelled by user.")
        self.win.tab2d.bar.setRange(0, 100)
        self.win.tab2d.bar.setValue(0)
        self.win.tab2d.bar.setStyleSheet("")

    def _show_cancel_confirmation(self) -> bool:
        """Shows a confirmation dialog and returns True if the user confirms."""
        step_display = "Unknown Step"
        if self._active_step_name and self._active_step_name in STEP_DISPLAY_NAMES:
            step_display = STEP_DISPLAY_NAMES[self._active_step_name]

        box = QMessageBox(self.win)
        box.setIcon(QMessageBox.Icon.Warning)
        box.setWindowTitle("Cancel Pipeline?")
        box.setTextFormat(Qt.TextFormat.RichText)
        box.setText(f"<b>Are you sure you want to cancel the pipeline?</b><br><br>The current step is: <i>{step_display}</i>")
        yes_btn = box.addButton("Yes", QMessageBox.ButtonRole.YesRole)
        box.addButton("No", QMessageBox.ButtonRole.NoRole)
        box.exec()
        return box.clickedButton() is yes_btn

    def request_cancel(self, parent_widget):
        """Handles a click on the 'Cancel' button."""
        if not self._job or not self._job.isRunning(): return
        if self._show_cancel_confirmation(): self._shutdown_pipeline()
    
    def emergency_shutdown(self, reason: str):
        self.log(f"UNCAUGHT EXCEPTION: {reason}", level="CRITICAL")
        self._stop_current_step()
        if self._job: self._job.cancel()
    
    # ========================================================================
    # CLOSE HANDLING
    # ========================================================================
    
    def handle_close(self, parent_widget, event) -> bool:
        """Handles the main window close event ('X' button)."""
        if self._job and self._job.isRunning():
            if not self._show_cancel_confirmation():
                return False

            self._shutdown_pipeline()
            self._job.wait()

        if self._session_active:
            box = QMessageBox(parent_widget)
            box.setIcon(QMessageBox.Icon.Information)
            box.setWindowTitle("End Session?")
            box.setText("<b>Closing will end this session.</b><br><br>"
                       "Rerun will not be available after closing.")
            close_btn = box.addButton("Close", QMessageBox.ButtonRole.YesRole)
            cancel_btn = box.addButton("Cancel", QMessageBox.ButtonRole.NoRole)
            box.setDefaultButton(close_btn)
            box.exec()
            
            if box.clickedButton() is not close_btn:
                return False
        
        self._stop_current_step()
        
        try:
            if self.audit.has_open_run(): self.audit.end_run("OK")
            self.log("GUI closed", level="DEBUG")
            if self.audit.has_open_session(): self.audit.end_session("Closed")
        except Exception:
            pass
        
        return True


