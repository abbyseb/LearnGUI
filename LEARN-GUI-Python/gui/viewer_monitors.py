# viewer_monitors.py
"""
Self-contained viewer monitors that watch train/ folder.
Automatically update viewers when files appear.
"""
from __future__ import annotations
from pathlib import Path
from typing import Optional, Dict
from .step_managers import get_projections_per_ct
import traceback
import numpy as np

from PyQt6.QtCore import QObject, QTimer, pyqtSignal


class ViewerMonitor(QObject):
    """Base class for file-watching viewer updaters."""
    
    files_found = pyqtSignal(list)
    
    def __init__(self, viewer_widget, parent=None):
        super().__init__(parent)
        self.viewer = viewer_widget
        self.train_dir: Optional[Path] = None
        self.is_active = False
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._poll)
        self._last_files = set()
        
        self.audit = None
        if parent and hasattr(parent, 'audit'):
            self.audit = parent.audit
    
    def _log(self, message: str, level: str = "ERROR"):
        if self.audit:
            self.audit.add(message, level=level)

    def start(self, train_dir: Path):
        self.train_dir = Path(train_dir)
        self.is_active = True
        self._last_files.clear()
        self.prepare_viewer()
        self._timer.start(self.get_poll_interval_ms())
        self._poll()
    
    def stop(self):
        self.is_active = False
        self._timer.stop()
    
    def _poll(self):
        if not self.is_active or not self.train_dir or not self.train_dir.exists():
            return
        
        try:
            current_files = set(self.find_files())
            if current_files != self._last_files:
                self._last_files = current_files
                files_list = sorted(current_files)
                self.update_viewer(files_list)
                self.files_found.emit(files_list)
        except Exception as e:
            self._log(f"Viewer monitor poll error in {self.__class__.__name__}: {e}")
    
    def get_poll_interval_ms(self) -> int:
        return 200
    
    def find_files(self) -> list[Path]:
        raise NotImplementedError
    
    def prepare_viewer(self):
        pass
    
    def update_viewer(self, files: list[Path]):
        raise NotImplementedError


class CTViewerMonitor(ViewerMonitor):
    """
    Monitors for CT_*.mha files and updates CTQuadPanel.
    Passes original sitk image for geometry-accurate overlay resampling.
    """
    
    def __init__(self, viewer_widget, parent=None):
        super().__init__(viewer_widget, parent)
        self._phase_connected = False
        self._all_files = []
        self._last_overlays: Dict[str, str] = {}
        # Cache stores (vol, spacing, origin, direction, sitk_image)
        self._volume_cache: Dict[Path, tuple] = {}
    
    def start(self, train_dir: Path):
        self._volume_cache.clear()
        super().start(train_dir)

    def get_poll_interval_ms(self) -> int:
        return 200
    
    def _poll(self):
        if not self.is_active or not self.train_dir or not self.train_dir.exists():
            return

        try:
            current_ct_files = set(self.find_files())
            if current_ct_files != self._last_files:
                self._last_files = current_ct_files
                self.update_viewer(sorted(current_ct_files))

            current_overlays = self._scan_overlays()
            if current_overlays != self._last_overlays:
                self._last_overlays = current_overlays
                self.viewer.set_overlay_sources(current_overlays)

        except Exception as e:
            self._log(f"CTViewerMonitor poll error: {e}")
    
    def prepare_viewer(self):
        try:
            self.viewer.prepare_for_volume_loading("Awaiting CT volumes...")
        except Exception:
            pass
    
    def find_files(self) -> list[Path]:
        if not self.train_dir or not self.train_dir.exists():
            return []
        return list(self.train_dir.glob("CT_*.mha"))
    
    def update_viewer(self, files: list[Path]):
        if not files:
            return
        
        try:
            from .ui.info_panels import CTInfoWidget

            self.viewer.set_mode("triptych")
            self._all_files = sorted(files)
            
            # Disable preview mode when MHA files are loaded
            if hasattr(self.viewer, 'info_panel') and isinstance(self.viewer.info_panel, CTInfoWidget):
                self.viewer.info_panel.set_preview_mode(False)

            if not self._phase_connected:
                try:
                    try: self.viewer.series_changed.disconnect()
                    except: pass
                    
                    self.viewer.series_changed.connect(self._on_phase_changed)
                    self._phase_connected = True
                except Exception as e:
                    self._log(f"Phase connection error: {e}", level="WARN")
            
            # Block signals during combo population to prevent race condition
            # This ensures _load_phase is only called once with correct state
            if len(self._all_files) > 1:
                names = [f.name for f in self._all_files]
                self.viewer.info_panel.cmb_phase.blockSignals(True)
                self.viewer.set_phase_options(names)
                self.viewer.info_panel.cmb_phase.blockSignals(False)
            
            current_idx = self.viewer.info_panel.cmb_phase.currentIndex()
            current_idx = max(0, current_idx)
            
            if current_idx >= len(self._all_files):
                current_idx = 0
            
            self._load_phase(current_idx)
            
        except Exception as e:
            error_details = traceback.format_exc()
            self._log(f"CT viewer update error: {e}\n{error_details}")
    
    def _on_phase_changed(self, index: int, _name: str):
        self._load_phase(index)
        self._check_and_set_overlays()
    
    def _load_phase(self, index: int):
        """Load specific CT phase, passing original sitk image for overlay accuracy."""
        if not self._all_files or index < 0 or index >= len(self._all_files):
            return
        
        try:
            from .preview import read_mha_volume
            
            file = self._all_files[index]
            if not file.exists():
                self._log(f"Phase file no longer exists: {file}", level="WARN")
                return

            # Check cache - now includes sitk_image
            if file in self._volume_cache:
                vol, spacing, origin, direction, sitk_img = self._volume_cache[file]
            else:
                # read_mha_volume now returns 5 values including sitk image
                vol, spacing, origin, direction, sitk_img = read_mha_volume(file)
                self._volume_cache[file] = (vol, spacing, origin, direction, sitk_img)
            
            # FIX: Pass sitk_image for geometry-accurate overlay resampling
            self.viewer.set_mha_volume(vol, spacing, origin, direction, sitk_image=sitk_img)
            
            self.viewer.set_info(
                source=f"MHA: {file.name}",
                dims=f"{vol.shape[0]} × {vol.shape[1]} × {vol.shape[2]}",
                vox=f"{spacing[0]:.3f}, {spacing[1]:.3f}, {spacing[2]:.3f}",
                count=str(vol.shape[0])
            )
            
        except Exception as e:
            error_details = traceback.format_exc()
            self._log(f"Phase load error: {e}\n{error_details}")

    def _check_and_set_overlays(self):
        current_overlays = self._scan_overlays()
        if current_overlays != self._last_overlays:
            self._last_overlays = current_overlays
            self.viewer.set_overlay_sources(current_overlays)
    
    def _scan_overlays(self) -> Dict[str, str]:
        overlays = {}
        candidates = {
            "Body": "Body.mha",
            "GTV_Inh": "GTV_Inh.mha",
            "GTV_Exh": "GTV_Exh.mha",
            "Lungs_Inh": "Lungs_Inh.mha",
            "Lungs_Exh": "Lungs_Exh.mha",
        }
        
        for name, filename in candidates.items():
            path = self.train_dir / filename
            if path.exists():
                overlays[name] = str(path)
        
        return overlays


class DRRViewerMonitor(ViewerMonitor):
    """Monitors for DRR .hnc files and updates DRRViewerPanel."""
    frame_ready = pyqtSignal(int, int, str, object, float)

    def __init__(self, viewer_widget, parent=None):
        super().__init__(viewer_widget, parent)
        self.frame_ready.connect(self.viewer.add_frame)

    def prepare_viewer(self):
        try:
            ct_count = self._get_ct_count()
            self.viewer.set_loading_state(ct_count)
            self.viewer.set_total_projections(get_projections_per_ct())
        except Exception:
            pass

    def find_files(self) -> list[Path]:
        if not self.train_dir or not self.train_dir.exists():
            return []
        # Support both .hnc and compressed .bin
        return list(self.train_dir.glob("*_Proj_*.hnc")) + list(self.train_dir.glob("*_Proj_*.bin"))
    
    def _poll(self):
        if not self.is_active or not self.train_dir or not self.train_dir.exists():
            return

        try:
            current_files = set(self.find_files())
            new_files = sorted(list(current_files - self._last_files))

            if new_files:
                self.update_viewer(new_files)
                self._last_files = current_files
        
        except Exception as e:
            self._log(f"DRRViewerMonitor poll error: {e}")

    def update_viewer(self, new_files: list[Path]):
        import re
        
        for hnc_path in new_files:
            if not self.is_active:
                break

            match = re.match(r"(\d+)_Proj_(\d+)\.hnc", hnc_path.name)
            if not match:
                continue

            ct_num = int(match.group(1))
            proj_num = int(match.group(2))
            ct_name = f"CT_{ct_num:02d}"

            if hnc_path.suffix == ".bin":
                img_array, angle = self._read_bin_file(hnc_path)
                angle = angle or 0.0 # Default if None
            else:
                img_array, angle = self._read_hnc_file(hnc_path)
            
            if img_array is None:
                continue

            pixmap = self._convert_to_pixmap(img_array)
            if pixmap and not pixmap.isNull():
                self.frame_ready.emit(ct_num, proj_num, ct_name, pixmap, angle)

    def _get_ct_count(self) -> int:
        if not self.train_dir or not self.train_dir.exists():
            return 0
        count = len(list(self.train_dir.glob("sub_CT_*.mha")))
        if count == 0:
            count = len(list(self.train_dir.glob("CT_*.mha")))
        return count

    def _read_bin_file(self, bin_path: Path) -> tuple:
        """Read 128x128 compressed binary projection."""
        try:
            import numpy as np
            # Compressed bin is always 128x128 single precision (4 bytes each)
            # 128*128*4 = 65536 bytes
            if bin_path.stat().st_size != 65536:
                return None, None
                
            img_data = np.fromfile(bin_path, dtype=np.float32)
            if len(img_data) == 16384: # 128 * 128
                img_array = img_data.reshape((128, 128), order='F')
                # The .bin files from compressProj.m already have log applied:
                # P = log(65536 ./ (P + 1));
                # We return as-is.
                return img_array, 0.0
            return None, None
        except Exception as e:
            self._log(f"Error reading BIN {bin_path.name}: {e}", level="WARN")
            return None, None

    def _read_hnc_file(self, hnc_path: Path) -> tuple:
        try:
            import struct
            import numpy as np

            if hnc_path.stat().st_size < 512:
                return None, None 

            with open(hnc_path, 'rb') as f:
                f.seek(120)
                size_x = struct.unpack('<I', f.read(4))[0]
                size_y = struct.unpack('<I', f.read(4))[0]
                if size_x == 0 or size_y == 0 or size_x > 4096 or size_y > 4096:
                    return None, None
                
                f.seek(368) 
                angle = struct.unpack('<d', f.read(8))[0]

                f.seek(512)
                pixel_count = size_x * size_y
                img_data = np.fromfile(f, dtype=np.uint16, count=pixel_count)
                if len(img_data) != pixel_count:
                    return None, None
                img_array = img_data.reshape((size_x, size_y), order='F')
                img_array = np.log(65536.0 / (img_array.astype(float) + 1.0))
                return img_array, angle
        except Exception as e:
            self._log(f"Error reading HNC {hnc_path.name}: {e}", level="WARN")
            return None, None

    def _convert_to_pixmap(self, img_array: np.ndarray) -> object:
        try:
            from PyQt6.QtGui import QImage, QPixmap
            wl, ww = 2.5, 3.0
            lo, hi = wl - ww / 2.0, wl + ww / 2.0
            windowed = np.clip(img_array, lo, hi)
            windowed = ((windowed - lo) / max(ww, 1e-6)) * 255.0
            u8_img = np.clip(windowed, 0, 255).astype(np.uint8)
            u8_img = np.transpose(u8_img)
            if not u8_img.flags['C_CONTIGUOUS']:
                u8_img = np.ascontiguousarray(u8_img)
            h, w = u8_img.shape
            qimg = QImage(u8_img.data, w, h, w, QImage.Format.Format_Grayscale8).copy()
            return QPixmap.fromImage(qimg)
        except Exception as e:
            self._log(f"Error converting DRR frame to pixmap: {e}")
            return None


class DVFViewerMonitor(ViewerMonitor):
    """Monitors for DVF files and updates DVFPanel."""
    
    def __init__(self, viewer_widget, mode: str = 'low', parent=None):
        super().__init__(viewer_widget, parent)
        self.mode = mode
        self._source_mha_path: Optional[Path] = None

    def start(self, train_dir: Path):
        from .preview import read_mha_volume
        
        super().start(train_dir)
        
        if self.mode == 'low':
            self._source_mha_path = self.train_dir / "sub_CT_06.mha"
            if not self._source_mha_path.exists():
                self._source_mha_path = self.train_dir / "source.mha"
            if not self._source_mha_path.exists():
                all_subs = sorted(list(self.train_dir.glob("sub_CT_*.mha")))
                if all_subs:
                    self._source_mha_path = all_subs[0]
                else:
                    self._source_mha_path = None
                    self._log("No downsampled CT found for DVF_LOW viewer.", "WARN")
        else:
            self._source_mha_path = self.train_dir / "source.mha"
            if not self._source_mha_path.exists():
                self._source_mha_path = self.train_dir / "CT_06.mha"
            if not self._source_mha_path.exists():
                all_full = sorted(list(self.train_dir.glob("CT_*.mha")))
                if all_full:
                    self._source_mha_path = all_full[0]
                else:
                    self._source_mha_path = None
                    self._log("No full-res CT found for DVF_FULL viewer.", "WARN")

        if self._source_mha_path and self._source_mha_path.exists():
            try:
                self._log(f"Loading DVF base CT: {self._source_mha_path.name}", "DEBUG")
                vol, spacing, origin, direction, sitk_img = read_mha_volume(self._source_mha_path)
                # Pass sitk_image for geometry accuracy
                self.viewer.set_mha_volume(vol, spacing, origin, direction, sitk_image=sitk_img)
            except Exception as e:
                self._log(f"Error loading source MHA for DVF viewer ({self._source_mha_path.name}): {e}")
        else:
            self._log(f"No source MHA found for DVF viewer (mode: {self.mode}).", "WARN")
            self.viewer.clear()
    
    def find_files(self) -> list[Path]:
        if not self.train_dir or not self.train_dir.exists():
            return []
        
        if self.mode == 'low':
            all_dvf = set(self.train_dir.glob("DVF_*.mha"))
            full_dvf = set(self.train_dir.glob("DVF_full_*.mha"))
            return list(all_dvf - full_dvf)
        else:
            return list(self.train_dir.glob("DVF_full_*.mha"))

    def update_viewer(self, files: list[Path]):
        dvf_map = {f.stem: str(f) for f in files} 
        self.viewer.set_dvf_sources(dvf_map)
        

class NoViewerMonitor(ViewerMonitor):
    """Dummy monitor for steps without viewers."""
    
    def find_files(self) -> list[Path]:
        return []
    
    def update_viewer(self, files: list[Path]):
        pass


def create_viewer_monitor(viewer_type: str, viewer_widget, parent=None) -> ViewerMonitor:
    """Create appropriate monitor for viewer type."""
    monitors = {
        'ct': CTViewerMonitor,
        'drr': DRRViewerMonitor,
        'dvf': DVFViewerMonitor,
        'none': NoViewerMonitor,
        'preparing': NoViewerMonitor,
    }
    
    monitor_class = monitors.get(viewer_type.lower(), NoViewerMonitor)
    
    kwargs = {}
    if monitor_class == DVFViewerMonitor:
        if parent and hasattr(parent, '_active_step_name'):
            if parent._active_step_name == 'DVF3D_FULL':
                kwargs['mode'] = 'full'
            else:
                kwargs['mode'] = 'low'
    
    return monitor_class(viewer_widget, parent=parent, **kwargs)