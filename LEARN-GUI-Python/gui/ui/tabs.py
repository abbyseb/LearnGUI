# ui/tabs.py - COMPLETE SESSION-BASED VERSION
"""
Pipeline step management UI with session-based rerun capability.

Key Features:
- Session activation on pipeline start
- Prerequisite-based rerun availability
- Dynamic step availability updates
- No run folder selection (uses current session)
"""

from typing import List, Dict
from PyQt6.QtWidgets import (
    QWidget, QGroupBox, QFormLayout, QLabel, QVBoxLayout,
    QHBoxLayout, QPushButton, QCheckBox, QProgressBar,
    QSizePolicy, QComboBox, QStackedWidget
)
from ..audit import AuditLog
from ..ui.panels import CTQuadPanel, DRRViewerPanel, DVFPanel

from PyQt6.QtGui import QDesktopServices
from PyQt6.QtCore import QUrl, Qt
from pathlib import Path


class Tab2D(QWidget):
    from PyQt6.QtCore import pyqtSignal
    run_pipeline_requested = pyqtSignal()
    continue_run_requested = pyqtSignal()
    rerun_step_requested = pyqtSignal(str)
    cancel_requested = pyqtSignal()
    rerun_refresh_requested = pyqtSignal()
    open_run_folder_requested = pyqtSignal()

    def __init__(self, audit: AuditLog, parent=None):
        super().__init__(parent)
        self.audit = audit
        self.step_labels: List[QLabel] = []
        self.step_checkboxes: List[QCheckBox] = []
        self._step_completed_status = [False] * 9
        self._step_stale_status = [False] * 9
        
        # Session state
        self._rerun_availability: Dict[str, Dict] = {}
        self._session_active = False

        root = QVBoxLayout(self)

        # ====================================================================
        # STEPS PANEL
        # ====================================================================
        
        steps = QGroupBox("Pipeline Steps")
        sv = QVBoxLayout(steps)
        sv.setSpacing(4)
        sv.setContentsMargins(8, 8, 8, 8)

        # Core steps header
        core_label = QLabel("<b>Core Steps</b>")
        core_label.setStyleSheet("color: #666; margin-top: 4px;")
        sv.addWidget(core_label)

        # Core steps (always required)
        self.step_texts = [
            "1. Copy training files",
            "2. DICOM → MHA",
            "3. Downsample volumes",
        ]
        
        for i, txt in enumerate(self.step_texts):
            row = QHBoxLayout()
            row.setSpacing(8)
            
            chk = QCheckBox()
            chk.setChecked(True)
            chk.setEnabled(True) # EDIT: Was False
            chk.setToolTip("Prerequisite: Patient selected") # EDIT: Updated tooltip
            chk.stateChanged.connect(lambda state, idx=i: self._on_step_toggled(idx, state))
            self.step_checkboxes.append(chk)
            
            lbl = QLabel(txt)
            self.step_labels.append(lbl)
            
            row.addWidget(chk)
            row.addWidget(lbl, 1)
            sv.addLayout(row)

        # 2D pathway header
        sv.addSpacing(12)
        pathway_label = QLabel("<b>2D Processing</b>")
        pathway_label.setStyleSheet("color: #666; margin-top: 4px;")
        sv.addWidget(pathway_label)

        # 2D processing steps (DRR + Compress checked by default for full pipeline)
        processing_2d_steps = [
            ("4. Generate DRRs", "Prerequisite: Downsample volumes"),
            ("5. Compress DRRs", "Prerequisite: Generate DRRs"),
            ("6. 2D DVFs (DRR Registration) - Optional", "Prerequisite: Compress DRRs"),
        ]
        
        for i, (txt, tooltip) in enumerate(processing_2d_steps, start=3):
            row = QHBoxLayout()
            row.setSpacing(8)
            
            chk = QCheckBox()
            # Steps 4 & 5 (DRR, Compress) checked by default; 6 (2D DVF) optional
            chk.setChecked(i < 5)
            chk.setToolTip(tooltip)
            chk.stateChanged.connect(lambda state, idx=i: self._on_step_toggled(idx, state))
            self.step_checkboxes.append(chk)
            
            lbl = QLabel(txt)
            self.step_labels.append(lbl)
            
            row.addWidget(chk)
            row.addWidget(lbl, 1)
            sv.addLayout(row)

        self.step_texts.extend([s[0] for s in processing_2d_steps])

        # 3D pathway header
        sv.addSpacing(12)
        pathway_3d_label = QLabel("<b>3D Processing</b>")
        pathway_3d_label.setStyleSheet("color: #666; margin-top: 4px;")
        sv.addWidget(pathway_3d_label)

        # 3D processing steps (only downsampled 3D DVF checked by default)
        processing_3d_steps = [
            ("7. 3D DVFs (Downsampled)", "Prerequisite: Downsample volumes"),
            ("8. 3D DVFs (Full Resolution)", "Prerequisite: DICOM → MHA"), # MODIFIED Tooltip
            ("9. kV TIFF → BIN (per fraction)", "Prerequisite: Copy training files"), # MODIFIED Tooltip
        ]
        
        for i, (txt, tooltip) in enumerate(processing_3d_steps, start=6):
            row = QHBoxLayout()
            row.setSpacing(8)
            
            chk = QCheckBox()
            # Step 7 (downsampled 3D DVF) checked by default; 8 (full res) and 9 (kV) optional
            chk.setChecked(i < 7)
            chk.setToolTip(tooltip)
            chk.stateChanged.connect(lambda state, idx=i: self._on_step_toggled(idx, state))
            self.step_checkboxes.append(chk)
            
            lbl = QLabel(txt)
            self.step_labels.append(lbl)
            
            row.addWidget(chk)
            row.addWidget(lbl, 1)
            sv.addLayout(row)

        self.step_texts.extend([s[0] for s in processing_3d_steps])

        steps.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Maximum)
        self.grp_steps = steps
        root.addWidget(self.grp_steps)

        # ====================================================================
        # RUN + RERUN CONTROLS (SESSION-BASED)
        # ====================================================================
        
        run_row = QHBoxLayout()

        # Run Pipeline button
        self.btn_run_pipeline = QPushButton("Run Pipeline")
        self.btn_run_pipeline.setMinimumWidth(120)
        run_row.addWidget(self.btn_run_pipeline)

        self.btn_continue_run = QPushButton("Continue run…")
        self.btn_continue_run.setToolTip(
            "Choose run workspace + optional custom log; creates Patient…/train if needed; "
            "skips steps that already have outputs (manifest or patient selection for prescriptions)."
        )
        run_row.addWidget(self.btn_continue_run)
        
        # Session info label
        self.lbl_session_info = QLabel("")
        self.lbl_session_info.setStyleSheet("color: #666; font-style: italic;")
        self.lbl_session_info.setVisible(False)
        run_row.addWidget(self.lbl_session_info)
        
        run_row.addSpacing(20)

        # Rerun controls (no folder selection)
        lbl_rerun_step = QLabel("Rerun step:")
        lbl_rerun_step.setStyleSheet("color: #666;")
        
        self.cbo_rerun = QComboBox()
        self.cbo_rerun.addItems([
            "Copy training files",
            "DICOM → MHA",
            "Downsample volumes",
            "Generate DRRs",
            "Compress DRRs",
            "2D DVFs",
            "3D DVFs (Downsampled)", # ADDED
            "3D DVFs (Full Resolution)", # ADDED
            "kV TIFF → BIN", # ADDED
        ])
        self.cbo_rerun.setMinimumWidth(180)
        
        self.btn_rerun_step = QPushButton("Rerun Step")
        self.btn_rerun_step.setEnabled(False)
        self.btn_rerun_step.setToolTip("Run pipeline first to enable rerun")

        # FIX: Add the Refresh button
        self.btn_refresh_rerun = QPushButton("⟳")
        self.btn_refresh_rerun.setToolTip("Refresh rerun step availability")
        self.btn_refresh_rerun.setFixedWidth(30)
        self.btn_refresh_rerun.setEnabled(False)
        
        run_row.addWidget(lbl_rerun_step)
        run_row.addWidget(self.cbo_rerun)
        run_row.addWidget(self.btn_rerun_step)
        run_row.addWidget(self.btn_refresh_rerun)

        run_row.addStretch(1)
        root.addLayout(run_row)

        # ====================================================================
        # PROGRESS SECTION
        # ====================================================================
        
        # Progress section
        prog = QGroupBox("Progress")
        pv = QVBoxLayout(prog)

        self.bar = QProgressBar()

        bar_row = QHBoxLayout()
        bar_row.addWidget(self.bar, 1)

        # Add right-side info label
        self.bar_info = QLabel("")
        self.bar_info.setMinimumWidth(200)
        self.bar_info.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        self.bar_info.setStyleSheet("color: #999; font-family: 'Courier New', monospace;")
        bar_row.addWidget(self.bar_info)

        pv.addLayout(bar_row)

        

        # MHA detail label (CT/Structure counts)
        self.lbl_mha_detail = QLabel("")
        self.lbl_mha_detail.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
        self.lbl_mha_detail.setStyleSheet("color: #b0b7c3;")
        self.lbl_mha_detail.setVisible(False)
        pv.addWidget(self.lbl_mha_detail)

        # Status label
        self.lbl = QLabel("Idle")
        pv.addWidget(self.lbl)
        root.addWidget(prog)

        # ====================================================================
        # ACTIONS
        # ====================================================================
        
        actions = QHBoxLayout()
        
        self.btn_cancel = QPushButton("Cancel")
        self.btn_cancel.setEnabled(False)
        actions.addStretch()
        actions.addWidget(self.btn_cancel)

        self.btn_open_folder = QPushButton("Open Run Folder")
        self.btn_open_folder.setEnabled(False)
        actions.addWidget(self.btn_open_folder)

        self.btn_open_log = QPushButton("Open Run Log")
        self.btn_open_log.clicked.connect(self._open_run_log)
        self.btn_open_log.setEnabled(False)
        actions.addWidget(self.btn_open_log)
        
        root.addLayout(actions)

        # ====================================================================
        # VIEWER AREA
        # ====================================================================
        
        self.viewer_stack = QStackedWidget()
        
        # CT viewer
        self.ct_quad = CTQuadPanel("Planning CT (Coronal ⟷ Axial ⟷ Sagittal)")
        
        # DRR viewer
        self.drr_viewer = DRRViewerPanel()

        # DVF viewer
        self.dvf_viewer = DVFPanel()
        
        # Preparing panel (shown during validation/purge)
        self.preparing_panel = QWidget()
        prep_layout = QVBoxLayout(self.preparing_panel)
        prep_label = QLabel("Preparing step...\n\nValidating prerequisites and setting up workspace.")
        prep_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        prep_label.setStyleSheet("color: #9aa0a6; font-size: 14pt;")
        prep_layout.addWidget(prep_label)
        
        self.viewer_stack.addWidget(self.ct_quad)
        self.viewer_stack.addWidget(self.drr_viewer)
        self.viewer_stack.addWidget(self.dvf_viewer)
        self.viewer_stack.addWidget(self.preparing_panel)
        root.addWidget(self.viewer_stack, 1)

        # ====================================================================
        # SIGNAL CONNECTIONS
        # ====================================================================
        
        self.btn_run_pipeline.clicked.connect(self.run_pipeline_requested.emit)
        self.btn_continue_run.clicked.connect(self.continue_run_requested.emit)
        self.btn_rerun_step.clicked.connect(
            lambda: self.rerun_step_requested.emit(self.rerun_step_token())
        )
        self.btn_cancel.clicked.connect(self.cancel_requested.emit)
        self.btn_refresh_rerun.clicked.connect(self.rerun_refresh_requested.emit)
        self.btn_open_folder.clicked.connect(self.open_run_folder_requested.emit)
        
        self._update_step_dependencies()

    # ========================================================================
    # SESSION MANAGEMENT
    # ========================================================================
    
    def activate_session(self):
        """Called when pipeline starts - enables rerun functionality."""
        self._session_active = True
        self.btn_rerun_step.setEnabled(True)
        self.btn_refresh_rerun.setEnabled(True)
        self.btn_open_folder.setEnabled(True)
        self.btn_rerun_step.setToolTip("Rerun steps become available as prerequisites are met")
        self.lbl_session_info.setText("Session active • Rerun available")
        self.lbl_session_info.setStyleSheet("color: #73c991; font-style: italic;")
        self.lbl_session_info.setVisible(True)
    
    def is_session_active(self) -> bool:
        """Check if session is active (rerun possible)."""
        return self._session_active
    
    def reset_session(self):
        """Reset session state (called when starting new pipeline)."""
        self._session_active = False
        self.btn_rerun_step.setEnabled(False)
        self.btn_refresh_rerun.setEnabled(False)
        self.btn_open_folder.setEnabled(False)
        self.btn_rerun_step.setToolTip("Run pipeline first to enable rerun")
        self.lbl_session_info.setVisible(False)
        self._rerun_availability.clear()

    def reset_viewers(self):
        """Reset all viewer panels to their initial empty state."""
        self.ct_quad.clear()
        self.drr_viewer.clear()
        self.dvf_viewer.clear() # Good practice to include this for the future
        self.switch_viewer('ct') # Switch back to the default CT viewer

    # ========================================================================
    # RERUN AVAILABILITY
    # ========================================================================
    
    def update_rerun_step_availability(self, availability: Dict[str, Dict]):
        """
        Update which rerun steps are available based on prerequisites.
        Called dynamically as steps complete.
        
        Args:
            availability: {step_display_name: {"available": bool, "reason": str}}
        """
        self._rerun_availability = availability
        self._update_rerun_step_display()
    
    def _update_rerun_step_display(self):
        """
        Update rerun dropdown to show availability.
        Steps are available if prerequisites exist, regardless of completion.
        """
        self.cbo_rerun.blockSignals(True)
        self.cbo_rerun.clear()
        
        steps = [
            "Copy training files",
            "DICOM → MHA",
            "Downsample volumes",
            "Generate DRRs",
            "Compress DRRs",
            "2D DVFs",
            "3D DVFs (Downsampled)", # ADDED
            "3D DVFs (Full Resolution)", # ADDED
            "kV TIFF → BIN", # ADDED
        ]
        
        for step in steps:
            if step in self._rerun_availability:
                avail = self._rerun_availability[step]
                if avail["available"]:
                    # Prerequisites exist - step is available
                    self.cbo_rerun.addItem(f"✓ {step}")
                    idx = self.cbo_rerun.count() - 1
                    # Show detailed tooltip with what's available
                    self.cbo_rerun.setItemData(idx, avail["reason"], Qt.ItemDataRole.ToolTipRole)
                else:
                    # Prerequisites missing - step unavailable
                    item_text = f"✗ {step}"
                    self.cbo_rerun.addItem(item_text)
                    idx = self.cbo_rerun.count() - 1
                    self.cbo_rerun.setItemData(idx, avail["reason"], Qt.ItemDataRole.ToolTipRole)
                    
                    # Visually indicate unavailable
                    model = self.cbo_rerun.model()
                    item = model.item(idx)
                    if item:
                        item.setEnabled(False)
            else:
                # Unknown availability - show normally
                self.cbo_rerun.addItem(step)
        
        self.cbo_rerun.blockSignals(False)
    
    def get_current_step_availability(self) -> Dict[str, Dict]:
        """Get current rerun availability status."""
        return self._rerun_availability

    # ========================================================================
    # LOG FILE ACCESS
    # ========================================================================
    
    def _open_run_log(self):
        """Open current run log via controller."""
        window = self.window()
        try:
            if hasattr(window, 'controller'):
                ctl = window.controller
                if hasattr(ctl, "audit") and hasattr(ctl.audit, "flush_log_file"):
                    ctl.audit.flush_log_file()
                log_path = ctl.run_log_path
                if log_path and Path(log_path).exists():
                    QDesktopServices.openUrl(QUrl.fromLocalFile(str(log_path)))
                else:
                    from PyQt6.QtWidgets import QMessageBox
                    QMessageBox.information(
                        self,
                        "No Log Available",
                        "No run log is currently available.\n\n"
                        "Start a pipeline run first."
                    )
        except Exception as e:
            print(f"Error opening log: {e}")

    # ========================================================================
    # STEP SELECTION & DEPENDENCIES
    # ========================================================================

    def _on_step_toggled(self, index: int, state: int):
        """Handle step checkbox toggle with dependency checking."""
        is_checked = (state == 2)
        
        # Block signals to prevent recursive updates
        for chk in self.step_checkboxes:
            chk.blockSignals(True)
        
        try:
            if is_checked:
                # Check prerequisites when enabling a step
                self._check_prerequisites(index)
            else:
                # Uncheck dependents when disabling a step
                self._uncheck_dependents(index)
            
            # Update all step states
            self._update_step_dependencies()
            
        finally:
            # Restore signals
            for chk in self.step_checkboxes:
                chk.blockSignals(False)
    
    def _check_prerequisites(self, index: int):
        """Recursively check and enable prerequisite steps."""
        prereqs = self._get_prerequisites(index)
        for prereq_idx in prereqs:
            if not self.step_checkboxes[prereq_idx].isChecked():
                self.step_checkboxes[prereq_idx].setChecked(True)
                self._check_prerequisites(prereq_idx)
    
    def _uncheck_dependents(self, index: int):
        """Recursively uncheck dependent steps."""
        dependents = self._get_dependents(index)
        for dep_idx in dependents:
            if self.step_checkboxes[dep_idx].isChecked():
                self.step_checkboxes[dep_idx].setChecked(False)
                self._uncheck_dependents(dep_idx)
    
    def _get_prerequisites(self, index: int) -> List[int]:
        """Get prerequisite step indices for a given step."""
        # This map defines the *direct* parent
        # The _check_prerequisites function will walk the chain
        prereq_map = {
            0: [],                      # Copy
            1: [0],                     # DICOM→MHA needs Copy
            2: [1],                     # Downsample needs DICOM→MHA
            3: [2],                     # DRR needs Downsample
            4: [3],                     # Compress needs DRR
            5: [4],                     # 2D DVF needs Compress
            6: [2],                     # 3D DVF Low needs Downsample
            7: [1],                     # 3D DVF Full needs DICOM→MHA
            8: [0],                     # kV process needs Copy (for patient folder)
        }
        return prereq_map.get(index, [])
    
    def _get_dependents(self, index: int) -> List[int]:
        """Get dependent step indices for a given step."""
        # This map defines the *direct* children
        # The _uncheck_dependents function will walk the chain
        dependent_map = {
            0: [1, 8],                  # Copy -> DICOM2MHA, KV_PREPROCESS
            1: [2, 7],                  # DICOM2MHA -> Downsample, DVF3DFull
            2: [3, 6],                  # Downsample -> DRR, DVF3DLow
            3: [4],                     # DRR -> Compress
            4: [5],                     # Compress -> DVF2D
            5: [],
            6: [],
            7: [],
            8: [],
        }
        return dependent_map.get(index, [])
    
    def _update_step_dependencies(self):
        """Update enabled state and tooltips based on dependencies."""
        for i, chk in enumerate(self.step_checkboxes):
            
            # Check if all prerequisites are met
            prereqs = self._get_prerequisites(i)
            all_prereqs_met = all(self.step_checkboxes[p].isChecked() for p in prereqs)
            
            if not all_prereqs_met:
                # Disable and uncheck if prerequisites missing
                chk.setEnabled(False)
                if chk.isChecked(): # Only uncheck if it was checked
                    chk.setChecked(False)
                missing = [self.step_texts[p] for p in prereqs 
                            if not self.step_checkboxes[p].isChecked()]
                if missing:
                    chk.setToolTip(f"Requires: {', '.join(missing)}")
            else:
                # Enable if prerequisites met
                chk.setEnabled(True)
                tooltips = {
                    0: "Prerequisite: Patient selected",
                    1: "Prerequisite: Copy training files",
                    2: "Prerequisite: DICOM → MHA",
                    3: "Prerequisite: Downsample volumes",
                    4: "Prerequisite: Generate DRRs",
                    5: "Prerequisite: Compress DRRs",
                    6: "Prerequisite: Downsample volumes",
                    7: "Prerequisite: DICOM → MHA",
                    8: "Prerequisite: Copy training files",
                }
                chk.setToolTip(tooltips.get(i, ""))
    
    def get_selected_steps(self) -> List[int]:
        """Get list of selected step indices."""
        return [i for i, chk in enumerate(self.step_checkboxes) if chk.isChecked()]
    
    def is_step_selected(self, index: int) -> bool:
        """Check if a specific step is selected."""
        if 0 <= index < len(self.step_checkboxes):
            return self.step_checkboxes[index].isChecked()
        return False

    # ========================================================================
    # UI HELPERS
    # ========================================================================
    
    def switch_viewer(self, name: str):
        """Switch active viewer panel."""
        # --- START FIX ---
        if name == 'drr':
            self.viewer_stack.setCurrentWidget(self.drr_viewer)
        elif name == 'dvf':
            self.viewer_stack.setCurrentWidget(self.dvf_viewer)
        # --- END FIX ---
        elif name == 'preparing':
            self.viewer_stack.setCurrentWidget(self.preparing_panel)
        else: # Default 'ct'
            self.viewer_stack.setCurrentWidget(self.ct_quad)

    def set_step_completed(self, index: int, completed: bool):
        """Mark a step as completed with ✓."""
        if not (0 <= index < len(self.step_labels)):
            return
        self._step_completed_status[index] = completed
        self._update_step_label(index)

    def set_step_stale(self, index: int, stale: bool):
        """Mark a step as stale (needs rerun)."""
        if not (0 <= index < len(self.step_labels)):
            return
        self._step_stale_status[index] = stale
        self._update_step_label(index)

    def _update_step_label(self, index: int):
        """Update step label appearance based on state."""
        if not (0 <= index < len(self.step_labels)):
            return
        
        lbl = self.step_labels[index]
        base_text = self.step_texts[index]
        is_complete = self._step_completed_status[index]
        is_stale = self._step_stale_status[index]
        
        # Build display text
        if is_complete:
            prefix = "✓ "
            if is_stale:
                prefix = "⚠ ✓ "
        else:
            prefix = ""
        
        lbl.setText(f"{prefix}{base_text}")
        
        # Apply styling
        if is_complete:
            if is_stale:
                lbl.setStyleSheet("color: #f39c12; font-weight: normal;")
            else:
                lbl.setStyleSheet("color: #73c991; font-weight: normal;")
        elif hasattr(self, '_current_active_step') and self._current_active_step == index:
            lbl.setStyleSheet("font-weight: 600; color: #3498db;")
        else:
            lbl.setStyleSheet("color: default; font-weight: normal;")

    def set_active_step(self, index: int | None):
        """Highlight currently active step."""
        self._current_active_step = index
        for i in range(len(self.step_labels)):
            self._update_step_label(i)

    def rerun_step_value(self) -> str:
        """Get rerun step display name (without prefix)."""
        text = self.cbo_rerun.currentText()
        # Remove leading symbols and space
        for prefix in ["✓ ", "✗ ", "○ "]:
            if text.startswith(prefix):
                return text[len(prefix):]
        return text

    def rerun_step_token(self) -> str:
        """Convert display name to internal step token."""
        text = self.rerun_step_value()
        lut = {
            "Copy training files": "COPY",
            "DICOM → MHA": "DICOM2MHA",
            "Downsample volumes": "DOWNSAMPLE",
            "Generate DRRs": "DRR",
            "Compress DRRs": "COMPRESS",
            "2D DVFs": "DVF2D",
            "3D DVFs (Downsampled)": "DVF3D_LOW",
            "3D DVFs (Full Resolution)": "DVF3D_FULL",
            "kV TIFF → BIN": "KV_PREPROCESS",
        }
        return lut.get(text, "COPY")

    def get_drr_generation_options(self) -> dict:
        """Settings from DRR tab → passed to generate_drrs on the next DRR pipeline step."""
        return self.drr_viewer.info_panel.get_drr_generation_options()

    def set_busy(self, running: bool):
        """Update UI during pipeline execution."""
        running = bool(running)
        self.btn_run_pipeline.setEnabled(not running)
        self.btn_continue_run.setEnabled(not running)
        
        # Rerun only disabled if actually running (not if session inactive)
        if running:
            self.btn_rerun_step.setEnabled(False)
            self.btn_refresh_rerun.setEnabled(False)
        else:
            self.btn_rerun_step.setEnabled(self._session_active)
            self.btn_refresh_rerun.setEnabled(self._session_active)
        
        self.cbo_rerun.setEnabled(not running and self._session_active)
        
        # Disable step checkboxes during run
        for i in range(len(self.step_checkboxes)): # MODIFIED
            self.step_checkboxes[i].setEnabled(not running)
        
        self.btn_cancel.setEnabled(running)
        self.btn_open_folder.setEnabled(self._session_active)
        self.btn_open_log.setEnabled(self._session_active)
        
        if running:
            self.bar.setRange(0, 0)
            self.lbl.setText("Running…")
            self.bar.setStyleSheet("")
        else:
            self.bar.setRange(0, 100)
            if not running:
                self._update_step_dependencies()

    # ========================================================================
    # PROGRESS UPDATES
    # ========================================================================

    def set_copy_counts(self, copied: int, total: int) -> None:
        """Display copy/conversion progress counts."""
        self.bar_info.setText(f"{copied}/{total}")

    def clear_copy_counts(self) -> None:
        """Clear progress count display."""
        self.bar_info.clear()

    def show_preparing(self, step_label: str) -> None:
        """Show preparing panel with step name."""
        try:
            self.switch_viewer('preparing')
            self.lbl.setText(f"Preparing: {step_label}…")
            self.bar.setRange(0, 0)
            self.clear_copy_counts()
            self.clear_mha_subcounts()
        except Exception:
            pass
    
    def reset_all_steps(self, from_index: int = 0):
        """Reset step states (completion, stale, active) from a given index."""
        # FIX: Iterate only from the specified index onwards.
        for i in range(from_index, len(self._step_completed_status)):
            self._step_completed_status[i] = False
            self._step_stale_status[i] = False
            self._update_step_label(i)

    def blank_all_viewers(self):
        """Forcibly clears all viewers and switches to a neutral 'preparing' panel."""
        self.ct_quad.clear()
        self.drr_viewer.clear()
        self.dvf_viewer.clear()
        self.switch_viewer('preparing') # Switch to the neutral blank panel
        self.ct_quad.repaint() # Force an immediate repaint to prevent stale images
        
        self.bar.setRange(0, 100)
        self.bar.setValue(0)
        self.lbl.setText("Starting...")
        self.clear_copy_counts()
        self.clear_mha_subcounts()

    def set_mha_subcounts(self, ct_done: int, ct_total: int, st_done: int, st_total: int) -> None:
        """Display DICOM→MHA detailed progress (CT + structures)."""
        try:
            ct_total_txt = "?" if ct_total is None or ct_total < 0 else str(ct_total)
            self.lbl_mha_detail.setText(f"CTs {ct_done} / {ct_total_txt} • Structures {st_done} / {st_total}")
            self.lbl_mha_detail.setVisible(True)
        except Exception:
            pass

    def clear_mha_subcounts(self) -> None:
        """Clear MHA detail display."""
        try:
            self.lbl_mha_detail.clear()
            self.lbl_mha_detail.setVisible(False)
        except Exception:
            pass

    def clear_bar_info(self):
            """Clear right-side progress info."""
            self.bar_info.clear()

