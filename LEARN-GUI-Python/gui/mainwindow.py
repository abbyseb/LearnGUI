# mainwindow.py
from __future__ import annotations
from pathlib import Path
from datetime import datetime
from PyQt6.QtCore import Qt, QEvent, QTimer
from PyQt6.QtGui import QGuiApplication
from PyQt6.QtWidgets import QMainWindow, QTabWidget, QDockWidget, QWidget, QMessageBox

from .audit import AuditLog
from .backend import WORK_ROOT
from .ui.tabs import Tab2D
from .ui.case_dock import CaseDock
from .controller import MainController
from .preview import read_mha_volume


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Voxelmap – Data Preparation")

        # Start maximized immediately
        self.setWindowState(self.windowState() | Qt.WindowState.WindowMaximized)

        # Workspace (path from backend.py, platform-dependent)
        self.work_root = Path(WORK_ROOT)
        self.work_root.mkdir(parents=True, exist_ok=True)
        
        # App-level audit log (controller manages run-specific logs)
        # Create a single, persistent session log for the application's lifecycle.
        logs_root = self.work_root / "logs"
        logs_root.mkdir(parents=True, exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        session_log_path = logs_root / f"session_{timestamp}.log"
        self.audit = AuditLog(session_log_path)

        # Center: Tabbed interface for viewers
        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(False)  # Tabs not closeable per requirements
        
        # Main tab (always present, shows active step)
        self.tab2d = Tab2D(self.audit)
        self.main_tab_index = self.tabs.addTab(self.tab2d, "Main")
        
        self.setCentralWidget(self.tabs)

        # Track viewer tabs created during pipeline
        self.viewer_tabs = []  # List of (tab_index, step_name, widget)


        # Enable stacked docks on the left
        self.setDockNestingEnabled(True)

        # --- Left: Case dock (top) ---
        self.case_dock = CaseDock(self.audit, self)
        self.case_dock.setObjectName("CaseDock")
        self.case_dock.setMinimumWidth(360)
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, self.case_dock)

        # --- Left: Steps dock (integrated 2D + 3D) ---
        self.steps_dock = None
        steps_widget = getattr(self.tab2d, "grp_steps", None)
        if steps_widget is not None:
            steps_widget.setParent(None)
            self.steps_dock = QDockWidget("", self)
            self.steps_dock.setObjectName("StepsDock")
            self.steps_dock.setAllowedAreas(Qt.DockWidgetArea.LeftDockWidgetArea)
            self.steps_dock.setFeatures(QDockWidget.DockWidgetFeature.NoDockWidgetFeatures)
            self.steps_dock.setTitleBarWidget(QWidget(self.steps_dock))
            self.steps_dock.setMinimumWidth(360)
            self.steps_dock.setWidget(steps_widget)
            self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, self.steps_dock)
            self.splitDockWidget(self.case_dock, self.steps_dock, Qt.Orientation.Vertical)

        # Controller (no longer needs run_log_path - manages per-run logs)
        self.controller = MainController(
            window=self,
            audit=self.audit,
            work_root=self.work_root,
        )

        # Wire signals
        self.case_dock.selection_changed.connect(self.controller.on_selection)
        self.tab2d.run_pipeline_requested.connect(self.controller.start_run_pipeline)
        self.tab2d.rerun_step_requested.connect(self.controller.start_rerun_step)
        self.tab2d.cancel_requested.connect(lambda: self.controller.request_cancel(self))
        self.tab2d.rerun_refresh_requested.connect(self.controller._check_rerun_availability)
        self.tab2d.open_run_folder_requested.connect(self.controller.open_run_folder)
        
        # NEW: Connect step completion signal for creating viewer tabs
        self.controller.step_viewer_completed.connect(self._create_viewer_tab)
        
        # --- FIX: Connect to the correct signal ---
        # This was the bug: it was connected to a non-existent signal.
        # It should connect to the controller's `pipeline_completed` signal.
        self.controller.pipeline_completed.connect(self._on_pipeline_complete)

        # Sensible minimum
        self.setMinimumSize(900, 650)
        self._did_split_resize = False

    def showEvent(self, e):
        super().showEvent(e)
        # Balance the left docks after window is realized
        if not self._did_split_resize and self.steps_dock is not None:
            self.setUpdatesEnabled(False)
            try:
                docks = [self.case_dock, self.steps_dock]
                sizes = [3, 4]
                self.resizeDocks(docks, sizes, Qt.Orientation.Vertical)
            finally:
                self.setUpdatesEnabled(True)
            self._did_split_resize = True

    def _create_viewer_tab(self, step_index: int, step_name: str, viewer_type: str):
        """
        Create a new tab with a cloned viewer when a step with visual output completes.
        Sets up independent monitor for interactivity.
        
        Args:
            step_index: Step number (1-based)
            step_name: Human-readable step name
            viewer_type: 'ct' or 'drr'
        """
        from .ui.panels import CTQuadPanel, DRRViewerPanel, DVFPanel
        from .viewer_monitors import CTViewerMonitor, DRRViewerMonitor, DVFViewerMonitor
        
        # Get train_dir once for all viewer types
        train_dir = self.controller._train_dir
        
        # Create new viewer instance
        if viewer_type == 'ct':
            new_viewer = CTQuadPanel(f"Step {step_index}: {step_name}")
            # Clone state from main viewer
            self._clone_ct_viewer_state(self.tab2d.ct_quad, new_viewer)
            
            # Create independent monitor for this tab
            if train_dir and train_dir.exists():
                monitor = CTViewerMonitor(new_viewer, parent=self)
                monitor.start(train_dir)
                # Store monitor reference
                new_viewer._monitor = monitor
            
        elif viewer_type == 'drr':
            new_viewer = DRRViewerPanel(f"Step {step_index}: {step_name}")
            # Clone state from main viewer
            self._clone_drr_viewer_state(self.tab2d.drr_viewer, new_viewer)
            
            # DRR viewers are static (frames already loaded), no monitor needed

        # --- START FIX: Added missing 'dvf' case ---
        elif viewer_type == 'dvf':
            new_viewer = DVFPanel(f"Step {step_index}: {step_name}")
            
            # Determine mode ('low' or 'full')
            is_full_res = "Full Resolution" in step_name
            mode = 'full' if is_full_res else 'low'
            
            # Clone state from main viewer
            self._clone_dvf_viewer_state(self.tab2d.dvf_viewer, new_viewer, mode)
            
            # Create independent monitor
            if train_dir and train_dir.exists():
                monitor = DVFViewerMonitor(new_viewer, mode=mode, parent=self)
                monitor.start(train_dir)
        # --- END FIX ---
            
        else:
            return
        
        # Insert tab before Main tab (so older tabs are on the left)
        tab_title = f"Step {step_index}: {step_name}"
        new_index = self.tabs.insertTab(self.main_tab_index, new_viewer, tab_title)
        
        # Update main tab index (it shifted right by 1)
        self.main_tab_index += 1
        
        # Track this viewer tab
        self.viewer_tabs.append((new_index, step_name, new_viewer))
        
        # Switch back to Main tab to show next step
        self.tabs.setCurrentIndex(self.main_tab_index)

    def _clone_ct_viewer_state(self, source, dest):
        """Clone CT viewer state by reloading from disk (proper initialization)."""
        try:
            from .preview import read_mha_volume
            
            # Copy phase options first (cmb_phase is on info_panel, not CTQuadPanel)
            if hasattr(source, 'info_panel') and hasattr(source.info_panel, 'cmb_phase'):
                cmb = source.info_panel.cmb_phase
                if cmb.count() > 0:
                    names = [cmb.itemText(i) for i in range(cmb.count())]
                    current_idx = cmb.currentIndex()
                    dest.set_phase_options(names, current_idx)
                    dest.info_panel.cmb_phase.setEnabled(cmb.isEnabled())
                    
                    # Get current phase file path
                    if current_idx >= 0 and self.controller and self.controller._train_dir:
                        phase_name = names[current_idx]
                        phase_file = self.controller._train_dir / phase_name
                        
                        # Reload from disk to trigger full initialization pipeline
                        if phase_file.exists():
                            vol, spacing, origin, direction, sitk_img = read_mha_volume(phase_file)
                            dest.set_mha_volume(vol, spacing, origin, direction, sitk_image=sitk_img)
                        
                        # Copy overlay sources BEFORE slice positions
                        if hasattr(source, '_overlay_sources'):
                            dest.set_overlay_sources(source._overlay_sources.copy())
                        
                        # Copy current slice indices
                        if hasattr(source, '_iS'):
                            dest._iS = source._iS
                            dest._iA = source._iA  
                            dest._iR = source._iR
                            dest._sync_sliders_from_state()
                        
                        # Force fresh overlay load: clear cache and reload
                        overlay_name = getattr(source, '_overlay_name', None)
                        if overlay_name:
                            # Clear any stale cached data
                            dest._overlay_cache.clear()
                            dest._overlay_tag = None
                            dest._ov_log = None
                            # Force reload with fresh resampling
                            dest.set_overlay(overlay_name)
                        else:
                            dest._update_triptych()
                        
                        # Multi-stage rescale for tab visibility
                        QTimer.singleShot(100, dest._rescale_all)
                        QTimer.singleShot(300, dest._rescale_all)
                    
        except Exception as e:
            print(f"Error cloning CT viewer: {e}")
            import traceback
            traceback.print_exc()

    def _clone_drr_viewer_state(self, source, dest):
        """Clone DRR viewer state from source to destination."""
        try:
            # FIX: Iterate through the source's data and load it into the destination
            for ct_idx, ct_data in source._ct_data.items():
                dest.add_ct_frames(
                    ct_idx,
                    ct_data.get('name', f'CT_{ct_idx:02d}'),
                    ct_data.get('files', {}).copy(),
                    ct_data.get('frames', {}).copy(),
                    ct_data.get('angles', {}).copy()
                )
            
            # Set the cloned viewer to the same position as the source
            if source._current_ct is not None:
                dest._display_ct(source._current_ct)
                dest._display_frame(source._current_frame)
                
        except Exception as e:
            print(f"Error cloning DRR viewer: {e}")

    def _clone_dvf_viewer_state(self, source, dest, mode):
        """Clone DVF viewer state from source to destination."""
        try:
            # 1. Manually load the correct base CT
            # Find the monitor of the *source* panel to get its base CT path
            source_monitor = None
            if hasattr(self.controller, '_current_viewer_monitor') and self.controller._current_viewer_monitor.viewer == source:
                source_monitor = self.controller._current_viewer_monitor

            if source_monitor and source_monitor._source_mha_path and source_monitor._source_mha_path.exists():
                try:
                    vol, spacing, origin, direction, sitk_img = read_mha_volume(source_monitor._source_mha_path)
                    dest.set_mha_volume(vol, spacing, origin, direction, sitk_image=sitk_img)
                except Exception as e:
                    self.controller.log(f"Clone DVF: Failed to load base CT: {e}", "ERROR")
            else:
                 self.controller.log(f"Clone DVF: Could not find source CT path for monitor.", "WARN")

            # 2. Copy the DVF options (dropdown contents)
            if hasattr(source, '_dvf_sources'):
                dvf_options = source._dvf_sources.copy()
                dest.set_dvf_sources(dvf_options)

            # 3. Copy the state
            # Copy rebind toggle
            if hasattr(source, 'chk_rebind'):
                dest.chk_rebind.setChecked(source.chk_rebind.isChecked())

            # Copy opacity
            if hasattr(source, 'sld_alpha'):
                dest.sld_alpha.setValue(source.sld_alpha.value())

            # Copy current DVF selection
            current_dvf_key = source.cmb_dvf.currentData()
            if current_dvf_key:
                new_index = dest.cmb_dvf.findData(current_dvf_key)
                if new_index >= 0:
                    dest.cmb_dvf.setCurrentIndex(new_index)
                    # _on_dvf_phase_changed will be triggered, loading the DVF

        except Exception as e:
            self.controller.log(f"Error cloning DVF viewer: {e}", "ERROR")
            import traceback
            traceback.print_exc()
    # --- END FIX ---

    def _on_pipeline_complete(self):
        """
        Called when the entire pipeline completes.
        The "Main" tab remains "Main".
        """
        # --- FIX: Removed logic that renamed the main tab ---
        self.controller.log("Pipeline complete. Main tab remains active.", level="DEBUG")

    def clear_viewer_tabs(self):
        """Remove all viewer tabs except Main (called when starting new pipeline)."""
        # Stop monitors for CT tabs
        for tab_idx, step_name, widget in self.viewer_tabs:
            if hasattr(widget, '_monitor'):
                try:
                    widget._monitor.stop()
                except:
                    pass
        
        # Remove tabs in reverse order to maintain indices
        for tab_idx, step_name, widget in reversed(self.viewer_tabs):
            self.tabs.removeTab(tab_idx)
            widget.deleteLater()
        
        self.viewer_tabs.clear()
        
        # Reset main tab index and name
        self.main_tab_index = 0
        self.tabs.setTabText(self.main_tab_index, "Main")

    def closeEvent(self, event):
        if not self.controller.handle_close(self, event):
            event.ignore()
            return
        event.accept()