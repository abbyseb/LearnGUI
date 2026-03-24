# step_managers.py
"""
Self-contained step managers for Voxelmap pipeline.
Each step knows:
- What files to watch for progress
- What viewer to activate
- When it's complete
- How to calculate percentage

Works identically for normal runs and reruns.
"""
from __future__ import annotations
from pathlib import Path
from typing import Optional, List
import re
from PyQt6.QtCore import pyqtSignal, QObject, QTimer
from .workers import (
    CopyWorker, Dicom2MhaWorker, DownsampleWorker, 
    DrrWorker, CompressWorker, DvfWorker, KvPreprocessWorker,
    Dvf2dWorker
)

def get_projections_per_ct() -> int:
    """Read projection count from RTKGeometry.xml or return default 120."""
    try:
        # relative to LEARN-GUI-Python/gui/step_managers.py
        # root is ../ (LEARN-GUI-Python)
        root = Path(__file__).resolve().parents[1]
        geom_path = root / "modules" / "drr_generation" / "RTKGeometry_360.xml"
        if geom_path.exists():
            content = geom_path.read_text(encoding='utf-8')
            # Each projection is marked by <Projection> tag
            count = len(re.findall(r"<Projection>", content))
            if count > 0:
                return count
    except Exception as e:
        print(f"Warning: Could not detect projection count: {e}")
    return 120 # Fallback


class StepManager(QObject):
    """
    Base class for step-specific progress management.
    
    Each step manager:
    - Monitors train/ for expected outputs
    - Calculates progress percentage
    - Activates appropriate viewer
    - Signals completion
    """
    
    progress_updated = pyqtSignal(int, str)  # percentage, status_text
    step_complete = pyqtSignal()
    activate_viewer = pyqtSignal(str)  # viewer_name: 'ct', 'drr', 'none'
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.train_dir: Optional[Path] = None
        self.is_active = False
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._poll)
        self._last_progress = -1
        self._completion_checks = 0
        self.worker = None
        
    def start(self, train_dir: Path):
        """Activate this step's monitoring and optionally its worker."""
        self.train_dir = Path(train_dir)
        self.is_active = True
        self._last_progress = -1
        self._completion_checks = 0
        
        # Activate viewer first
        viewer = self.get_viewer_type()
        if viewer:
            self.activate_viewer.emit(viewer)
        
        # Instantiate specialized worker if one is defined for this step
        self._start_worker()
        
        # Standardize worker signal handling if a worker was created
        if self.worker:
            self.worker.finished_ok.connect(self._on_worker_finished)
            self.worker.failed.connect(self._on_worker_failed)
            # Start the worker thread AFTER connecting signals
            self.worker.start()
        
        # Start polling (for UI progress feedback)
        interval = self.get_poll_interval_ms()
        self._timer.start(interval)
        
        # Immediate first poll
        self._poll()
    
    def _start_worker(self):
        """Override in subclasses to start a QThread worker."""
        pass

    def stop(self):
        """Deactivate monitoring and stop worker."""
        self.is_active = False
        self._timer.stop()
        if self.worker and self.worker.isRunning():
            self.worker.terminate() # Or safer cancellation if implemented
            self.worker.wait()
    
    def _poll(self):
        """Poll train directory and emit progress."""
        if not self.is_active or not self.train_dir:
            return
        
        try:
            created, total, pct = self.calculate_progress()
            status = self.format_status(created, total)
            
            # Emit if changed
            if pct != self._last_progress:
                self.progress_updated.emit(pct, status)
                self._last_progress = pct
            
            # Check completion (require 3 consecutive polls at 100%)
            if self.is_complete(created, total):
                self._completion_checks += 1
                if self._completion_checks >= 3:
                    self.stop()
                    self.step_complete.emit()
            else:
                self._completion_checks = 0
                
        except Exception as e:
            print(f"Poll error in {self.__class__.__name__}: {e}")
    
    # Abstract methods - override in subclasses
    def get_viewer_type(self) -> Optional[str]:
        """Return 'ct', 'drr', or None."""
        return None
    
    def get_poll_interval_ms(self) -> int:
        """Poll frequency in milliseconds."""
        return 250
    
    def calculate_progress(self) -> tuple[int, int, int]:
        """Return (created_count, total_count, percentage)."""
        raise NotImplementedError
    
    def format_status(self, created: int, total: int) -> str:
        """Format status text for progress bar."""
        raise NotImplementedError
    
    def is_complete(self, created: int, total: int) -> bool:
        """Check if step is logically done."""
        if total <= 0:
            return True
        return created >= total

    def _on_worker_finished(self, msg: str):
        """Standard handler for worker success."""
        if self.is_active:
            # Force one last poll to ensure 100% UI
            self._poll()
            self.stop()
            self.step_complete.emit()

    def _on_worker_failed(self, error: str):
        """Standard handler for worker failure."""
        if self.is_active:
            self.stop()
            # Most steps will pass the error up via signals if needed, 
            # but simple manager stop is the baseline.
            # (In MainController, we rely on the specific manager signals or worker signals)


class CopyStepManager(StepManager):
    """Monitors DICOM file copying with efficient DICOM preview."""
    
    progress_detail = pyqtSignal(int, int)  # copied, total (for right label)
    
    def __init__(self, total_dicoms: int, parent=None):
        super().__init__(parent)
        self.total_dicoms = total_dicoms
        self.baseline_count = 0
        self._preview_engine = None
    
    def start(self, train_dir: Path):
        # Record baseline before starting
        self.baseline_count = self._count_dicoms(train_dir)
        super().start(train_dir)
    
    def _start_worker(self):
        """Start the Python copy worker directly."""
        controller = self.parent()
        if not controller or not hasattr(controller, "run_params"):
            return
            
        # Get params from controller (where user selection is stored)
        params = controller._run_params # Accessing controller's state
        self.worker = CopyWorker(
            params['patient_id'],
            params['rt_list'],
            params['src_base'],
            params['dst_base']
        )
        
        # Connect preview signal directly to our handler
        self.worker.preview_frame_ready.connect(self._on_push_preview)
        
        # DO NOT call self.worker.start() here; base class handles it
    
    def stop(self):
        super().stop()
        # Decommissioned engine cleanup (for safety)
        if hasattr(self, '_preview_engine') and self._preview_engine:
            try:
                self._preview_engine.stop()
            except Exception:
                pass
            self._preview_engine = None
    
    def get_viewer_type(self) -> Optional[str]:
        # Return None to prevent the main controller from switching the viewer.
        # The Copy step manages its own DICOM preview on the initial panel.
        return None
    
    def calculate_progress(self) -> tuple[int, int, int]:
        current = self._count_dicoms(self.train_dir)
        copied = max(0, current - self.baseline_count)
        total = self.total_dicoms
        pct = int(min(100, (copied / total * 100))) if total > 0 else 0
        
        # Emit detail signal for right-side label
        self.progress_detail.emit(copied, total)
        
        return copied, total, pct
    
    def format_status(self, created: int, total: int) -> str:
        return "Copying training files..."
    
    def _on_push_preview(self, pix: object, info: dict):
        """Handle preview frames pushed from the worker."""
        try:
            controller = self.parent()
            if not hasattr(controller, "win"): return
            viewer = controller.win.tab2d.ct_quad
            
            # Update viewer
            from PyQt6.QtGui import QPixmap
            if isinstance(pix, QPixmap) and not pix.isNull():
                viewer.set_axial_pixmap(pix)
            if info:
                viewer.set_info_dict(info)
        except Exception:
            pass

    def _start_dicom_preview(self):
        """Start efficient DICOM preview engine."""
        try:
            from .dicom_preview import DICOMPreviewEngine
            from .ui.info_panels import CTInfoWidget 
            
            # Get viewer reference
            controller = self.parent()
            if not hasattr(controller, 'win'):
                return
            
            viewer = controller.win.tab2d.ct_quad
            
            # Enable Preview Mode (Hide complex controls)
            if hasattr(viewer, 'info_panel') and isinstance(viewer.info_panel, CTInfoWidget):
                viewer.info_panel.set_preview_mode(True)
            
            # Create efficient preview engine
            self._preview_engine = DICOMPreviewEngine(
                self.train_dir,
                display_fps=8,  # Slightly faster for smoother carousel
                discovery_interval_ms=300,  # Less frequent to save CPU
                logger=controller.log,  # Pass controller's logger
                parent=self
            )
            
            # Check pydicom availability immediately
            import pydicom
            if pydicom is None:
                controller.log("ERROR: pydicom not found! DICOM preview will be disabled.", level="ERROR")
            else:
                controller.log("DICOM preview engine initialized.", level="DEBUG")
            
            # Connect to viewer updates (only axial visible in single/preview mode)
            def update_preview(pix_ax, pix_co, pix_sa, info):
                try:
                    if pix_ax and not pix_ax.isNull():
                        viewer.set_axial_pixmap(pix_ax)
                    if info:
                        viewer.set_info_dict(info)
                except Exception as e:
                    print(f"Preview update error: {e}")
            
            self._preview_engine.frame_ready.connect(update_preview)
            self._preview_engine.start()
            
        except Exception as e:
            print(f"DICOM preview engine failed: {e}")
            import traceback
            traceback.print_exc()
    
    @staticmethod
    def _count_dicoms(path: Path) -> int:
        """Count DICOM files recursively, including extensionless ones."""
        if not path or not path.exists():
            return 0
        count = 0
        for p in path.rglob("*"):
            if p.is_file():
                ext = p.suffix.lower()
                if ext in {".dcm", ".dicom", ".ima"}:
                    count += 1
                elif ext == "":
                    # Check for DICM magic bytes at offset 128
                    try:
                        with open(p, "rb") as fh:
                            fh.seek(128)
                            if fh.read(4) == b"DICM":
                                count += 1
                    except Exception:
                        pass
        return count



class DICOM2MHAStepManager(StepManager):
    """Monitors DICOM → MHA conversion."""
    
    progress_detail = pyqtSignal(int, int, int, int)  # ct_done, ct_total, struct_done, struct_total
    
    def get_viewer_type(self) -> Optional[str]:
        return 'ct'
    
    def get_poll_interval_ms(self) -> int:
        return 500  # MHA files are large, slower polling is fine
    
    def _start_worker(self):
        """Start the Python DICOM2MHA worker directly."""
        if not self.train_dir: return
        
        # dicom_src is train_dir (where COPY put them)
        dicom_src = self.train_dir
        # sample_path is just used as a base for directory resolving in worker
        sample_path = str(self.train_dir / "CT_01.mha")
        
        self.worker = Dicom2MhaWorker(str(dicom_src), sample_path)
        # self.worker.start() -> removed, base class handles it
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, -1, 0
        
        # Count created MHAs
        ct_created = len(list(self.train_dir.glob("CT_*.mha")))
        struct_names = ["Body.mha", "GTV_Inh.mha", "GTV_Exh.mha", "Lungs_Inh.mha", "Lungs_Exh.mha"]
        struct_found = [n for n in struct_names if (self.train_dir / n).exists()]
        struct_created = len(struct_found)
        
        # Try to get expected count from SeriesInfo.txt
        ct_expected = self._read_expected_ct_count()
        if ct_expected is None:
            # Indeterminate state
            self.progress_detail.emit(ct_created, -1, struct_created, 5)
            return ct_created, -1, 0
        
        # Emit detail for right label
        self.progress_detail.emit(ct_created, ct_expected, struct_created, 5)
        
        # Weighted progress
        # Many datasets (like VALKIM) don't have all structures. 
        # We give 90% weight to CTs and 10% to "any" structure found.
        ct_pct = (ct_created / ct_expected) * 90 if ct_expected > 0 else 0
        struct_pct = 10 if struct_created > 0 else 0
        
        pct = int(ct_pct + struct_pct)
        
        # If we have all CTs, we are at least at 90%
        if ct_created >= ct_expected and pct < 90:
            pct = 90

        total = ct_expected + 5
        created = ct_created + struct_created
        
        return created, total, pct
    
    def format_status(self, created: int, total: int) -> str:
        if total == -1:
            return "Converting DICOM → MHA... (scanning)"
        return "Converting DICOM → MHA..."
    
    def _read_expected_ct_count(self) -> Optional[int]:
        """Parse SeriesInfo.txt for expected CT count."""
        series_info = self.train_dir / "SeriesInfo.txt"
        if not series_info.exists():
            return None
        try:
            import re
            count = 0
            with series_info.open("r", encoding="utf-8", errors="ignore") as f:
                for line in f:
                    if re.search(r"\bCT_\d{2}\s*:", line):
                        count += 1
            return count if count > 0 else None
        except Exception:
            return None


class DownsampleStepManager(StepManager):
    """Monitors downsampling (no viewer change - lingers on CT)."""
    
    progress_detail = pyqtSignal(int, int)  # created, total
    
    def get_viewer_type(self) -> Optional[str]:
        return None  # Keep previous viewer
    
    def get_poll_interval_ms(self) -> int:
        return 1000
    
    def _start_worker(self):
        """Start the Python DOWNSAMPLE worker."""
        if not self.train_dir: return
        self.worker = DownsampleWorker(self.train_dir)
        # self.worker.start() -> removed, base class handles it

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        # Expected: all CT_*.mha + Lungs_*.mha
        expected_inputs = (len(list(self.train_dir.glob("CT_*.mha"))) +
                          len(list(self.train_dir.glob("Lungs_*.mha"))))
        
        # Created: sub_CT_*.mha + sub_Lungs_*.mha
        created_outputs = (len(list(self.train_dir.glob("sub_CT_*.mha"))) +
                          len(list(self.train_dir.glob("sub_Lungs_*.mha"))))
        
        # Emit detail
        self.progress_detail.emit(created_outputs, expected_inputs)
        
        pct = int(min(100, (created_outputs / expected_inputs * 100))) if expected_inputs > 0 else 0
        
        return created_outputs, expected_inputs, pct
    
    def format_status(self, created: int, total: int) -> str:
        return "Downsampling volumes..."


class DRRStepManager(StepManager):
    """Monitors DRR generation with per-CT progress."""
    
    # We detect this dynamically now
    _proj_cache: Optional[int] = None
    
    @property
    def projections_per_ct(self) -> int:
        if DRRStepManager._proj_cache is None:
            DRRStepManager._proj_cache = get_projections_per_ct()
        return DRRStepManager._proj_cache

    progress_detail = pyqtSignal(str)  # Detailed status like "CT_01/10 • 120/720"
    
    def get_viewer_type(self) -> Optional[str]:
        return 'drr'
    
    def get_poll_interval_ms(self) -> int:
        return 300
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        # Group files by CT
        ct_groups = self._group_by_ct()
        total_cts = self._get_ct_count()
        
        if not ct_groups or total_cts == 0:
            self.progress_detail.emit(f"Waiting for CTs... (0/{total_cts})")
            return 0, 0, 0
        
        # Count total created
        total_created = sum(len(files) for files in ct_groups.values())
        total_expected = total_cts * self.projections_per_ct
        
        # Find current CT being processed
        completed_cts = sum(1 for files in ct_groups.values() 
                          if len(files) >= self.projections_per_ct)
        
        current_ct = None
        current_count = 0
        for ct_num in sorted(ct_groups.keys()):
            files = ct_groups[ct_num]
            if len(files) < self.projections_per_ct:
                current_ct = ct_num
                current_count = len(files)
                break
        
        # Build detail string
        if current_ct:
            detail = f"CT_{current_ct:02d}/{total_cts:02d} • {current_count}/{self.projections_per_ct}"
        elif completed_cts == total_cts:
            detail = f"Complete • {total_cts} CTs"
        else:
            detail = f"Waiting... ({completed_cts}/{total_cts} CTs done)"
        
        self.progress_detail.emit(detail)
        
        pct = int(min(100, (total_created / total_expected * 100))) if total_expected > 0 else 0
        
        return total_created, total_expected, pct
    
    def format_status(self, created: int, total: int) -> str:
        return "Generating DRRs..."
    
    def _group_by_ct(self) -> dict:
        """Group HNC/BIN files by CT number."""
        import re
        groups = {}
        
        # Support both .hnc (generation) and .bin (post-compression)
        patterns = ["*_Proj_*.hnc", "*_Proj_*.bin"]
        
        for pattern in patterns:
            for file in self.train_dir.glob(pattern):
                match = re.search(r"(\d+)_Proj_(\d+)\.(hnc|bin)", file.name)
                if match:
                    ct_num = int(match.group(1))
                    if ct_num not in groups:
                        groups[ct_num] = set()
                    groups[ct_num].add(file.name)
        
        return groups
    
    def _get_ct_count(self) -> int:
        """Get CT count from downsampled volumes."""
        return len(list(self.train_dir.glob("sub_CT_*.mha")))


class CompressStepManager(StepManager):
    """Monitors DRR compression."""
    
    progress_detail = pyqtSignal(int, int)  # created, total
    
    def get_poll_interval_ms(self) -> int:
        return 1000
    
    def _start_worker(self):
        """Start the Python COMPRESS worker."""
        if not self.train_dir: return
        self.worker = CompressWorker(self.train_dir)
        # self.worker.start() -> removed, base class handles it

    @property
    def projections_per_ct(self) -> int:
        return get_projections_per_ct()

    def get_viewer_type(self) -> Optional[str]:
        return None  # Keep DRR viewer
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        # Total projections equals total CTs * dynamic count
        total_cts = len(list(self.train_dir.glob("sub_CT_*.mha")))
        ppc = self.projections_per_ct
        expected = total_cts * ppc if total_cts > 0 else ppc * 10 
        
        # Created: .bin files (the result of compression)
        created = len(list(self.train_dir.glob("*_Proj_*.bin")))
        
        # Emit detail
        self.progress_detail.emit(created, expected)
        
        pct = int(min(100, (created / expected * 100))) if expected > 0 else 0
        
        return created, expected, pct

    def format_status(self, created: int, total: int) -> str:
        return f"Compressing DRRs... ({created}/{total})"


class DVF2DStepManager(StepManager):
    """Monitors 2D DVF generation."""
    
    def get_viewer_type(self) -> Optional[str]:
        return None  # Keep DRR viewer
    
    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0
        
        # Look for *2DDVF* files
        created = len(list(self.train_dir.glob("*2DDVF*")))
        
        # Expected is proportional to BIN/HNC projections
        projs = (len(list(self.train_dir.glob("*_Proj_*.bin"))) + 
                 len(list(self.train_dir.glob("*_Proj_*.hnc"))))
        
        if projs > 0 and created > 0:
            pct = min(100, int((created / projs) * 100))
        else:
            pct = 0
        
        return created, projs, pct
    
    def _start_worker(self):
        """Start the Python DRR worker."""
        if not self.train_dir: return
        self.worker = DrrWorker(self.train_dir)
        # self.worker.start() -> removed, base class handles it

    def format_status(self, created: int, total: int) -> str:
        return f"Generating 2D DVFs..."
    
    def is_complete(self, created: int, total: int) -> bool:
        # Complete when files exist and haven't changed for several polls
        return created > 0 and self._completion_checks >= 5


class DVF3DLowStepManager(StepManager):
    """Monitors 3D DVF (downsampled) generation."""
    
    progress_detail = pyqtSignal(int, int)  # created, total

    def _start_worker(self):
        """Start the Python 3D DVF (low-res) worker."""
        if not self.train_dir: return
        self.worker = DvfWorker(self.train_dir, do_low=True, do_full=False)
        # self.worker.start() -> removed, base class handles it

    def get_viewer_type(self) -> Optional[str]:
        return 'dvf'

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0

        # Total is the number of downsampled CTs (10) + Lungs (2)
        total_cts = len(list(self.train_dir.glob("sub_CT_*.mha")))
        total_lungs = len(list(self.train_dir.glob("sub_Lungs_*.mha")))
        total = total_cts + total_lungs
        if total == 0:
            total = 12 # Default guess if inputs aren't visible yet

        # Created files are all DVFs *excluding* full-res
        all_dvf = set(self.train_dir.glob("DVF_*.mha"))
        full_dvf = set(self.train_dir.glob("DVF_full_*.mha"))
        created = len(all_dvf - full_dvf)
        
        self.progress_detail.emit(created, total)
        
        pct = int(min(100, (created / total * 100))) if total > 0 else 0
        return created, total, pct

    def format_status(self, created: int, total: int) -> str:
        return "Generating 3D DVFs (downsampled)..."

class DVF3DFullStepManager(StepManager):
    """Monitors 3D DVF (full-res) generation."""
    
    progress_detail = pyqtSignal(int, int)  # created, total

    def _start_worker(self):
        """Start the Python 3D DVF (full-res) worker."""
        if not self.train_dir: return
        self.worker = DvfWorker(self.train_dir, do_low=False, do_full=True)
        # self.worker.start() -> removed, base class handles it

    def get_viewer_type(self) -> Optional[str]:
        return 'dvf'

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir or not self.train_dir.exists():
            return 0, 0, 0

        # Total is the number of full-res volumes to be processed.
        total_cts = len(list(self.train_dir.glob("CT_*.mha")))
        total = total_cts if total_cts > 0 else 10 # Default guess

        # Created files are *only* the DVF_full_*.mha
        created = len(list(self.train_dir.glob("DVF_full_*.mha")))
        
        self.progress_detail.emit(created, total)
        
        pct = int(min(100, (created / total * 100))) if total > 0 else 0
        return created, total, pct

    def format_status(self, created: int, total: int) -> str:
        return "Generating 3D DVFs (full res)..."


class KVPreprocessStepManager(StepManager):
    """Monitors test data copy and kV preprocessing."""
    
    progress_detail = pyqtSignal(int, int) # copied/processed, total_fractions
    
    def __init__(self, rt_ct_pres_list: list, base_dir: str, parent=None):
        super().__init__(parent)
        self.rt_ct_pres_list = rt_ct_pres_list
        self.base_dir = base_dir
        self.total_fractions = 0
    
    def _start_worker(self):
        """Start the Python KV PREPROCESS worker."""
        if not self.train_dir: return
        self.worker = KvPreprocessWorker()
        self.worker.start()

    def get_viewer_type(self) -> Optional[str]:
        return "preparing"
        
    def start(self, train_dir: Path):
        self.total_fractions = self._find_total_fractions()
        super().start(train_dir)
        
    def _find_total_fractions(self) -> int:
        """Find expected fraction folders from source."""
        from .run_preparation import train_src_from_rt
        import re
        count = 0
        if not self.rt_ct_pres_list:
            return 1 # Avoid divide by zero
            
        rt = self.rt_ct_pres_list[0]
        # Get /VALKIM/RNSH/PlanningFiles/Patient01/
        planning_files_dir = train_src_from_rt(rt, self.base_dir).parent.parent
        treatment_files_dir = planning_files_dir.parent / "Treatment files"
        
        if not treatment_files_dir.exists():
            return 1
            
        try:
            for item in treatment_files_dir.iterdir():
                if item.is_dir() and re.match(r"^Fx\d+$", item.name, re.IGNORECASE):
                    count += 1
        except Exception:
            pass
        return count if count > 0 else 1

    def calculate_progress(self) -> tuple[int, int, int]:
        if not self.train_dir:
            return 0, 1, 0
            
        test_dir = self.train_dir.parent / "test"
        if not test_dir.exists():
            return 0, self.total_fractions, 0
        
        # Monitor .bin files as indicator of progress
        created = len(list(test_dir.rglob("*.bin")))
        
        # Count processed folders
        processed_folders = set()
        for bin_file in test_dir.rglob("*.bin"):
            try:
                processed_folders.add(bin_file.parent.parent.name)
            except Exception:
                pass
        
        processed_count = len(processed_folders)
        self.progress_detail.emit(processed_count, self.total_fractions)
        
        pct = int(min(100, (processed_count / self.total_fractions * 100)))
        return processed_count, self.total_fractions, pct

    def format_status(self, created: int, total: int) -> str:
        return f"Copying/Processing kV files..."


# Factory function
def create_step_manager(step_name: str, **kwargs) -> StepManager:
    """Create appropriate manager for step."""
    manager_class = {
        'COPY': CopyStepManager,
        'DICOM2MHA': DICOM2MHAStepManager,
        'DOWNSAMPLE': DownsampleStepManager,
        'DRR': DRRStepManager,
        'COMPRESS': CompressStepManager,
        'DVF2D': DVF2DStepManager,
        'DVF3D_LOW': DVF3DLowStepManager,
        'DVF3D_FULL': DVF3DFullStepManager,
        'KV_PREPROCESS': KVPreprocessStepManager,
    }.get(step_name.upper())
    
    if not manager_class:
        raise ValueError(f"Unknown step: {step_name}")
    
    # Pass only the arguments the constructor expects
    import inspect
    sig = inspect.signature(manager_class.__init__)
    # 1. Pop 'parent' from kwargs. It's passed explicitly.
    parent_arg = kwargs.pop('parent', None)
    
    # 2. Filter the *remaining* kwargs against the signature
    valid_kwargs = {k: v for k, v in kwargs.items() if k in sig.parameters}
    
    # 3. Call with the separated parent and valid_kwargs
    return manager_class(parent=parent_arg, **valid_kwargs)