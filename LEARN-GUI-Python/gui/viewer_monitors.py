# viewer_monitors.py
"""
Self-contained viewer monitors that watch train/ folder.
Automatically update viewers when files appear.
"""
from __future__ import annotations
from pathlib import Path
from typing import Dict, Optional, Set
from .step_managers import get_projections_per_ct
import traceback
import numpy as np
import SimpleITK as sitk
from PIL import Image
from PyQt6.QtCore import Qt, QObject, pyqtSignal, QTimer
from PyQt6.QtGui import QPixmap, QImage


def _u8_to_grayscale_pixmap(u8: np.ndarray):
    """Row-major uint8 H×W → QPixmap (same buffer layout as Qt Format_Grayscale8)."""
    u8 = np.ascontiguousarray(u8)
    if u8.ndim != 2:
        return None
    h, w = u8.shape
    qimg = QImage(u8.data, w, h, w, QImage.Format.Format_Grayscale8).copy()
    return QPixmap.fromImage(qimg)


def _prepare_png_uint8_for_display(arr: np.ndarray) -> np.ndarray:
    """
    Clip PNG payload to uint8, C-contiguous. Default: no transpose (matches cv2-written
    Joseph DRR slices, typically 768×1024). LEARNGUI_DRR_VIEW_PNG_TRANSPOSE=1 swaps axes.
    """
    import os

    u8 = np.clip(arr, 0, 255).astype(np.uint8)
    u8 = np.ascontiguousarray(u8)
    if u8.ndim != 2:
        return u8
    if os.environ.get("LEARNGUI_DRR_VIEW_PNG_TRANSPOSE", "0").strip().lower() in (
        "1",
        "true",
        "yes",
        "on",
    ):
        u8 = np.ascontiguousarray(u8.T)
    return u8


class ViewerMonitor(QObject):
    """Base class for file-watching viewer updaters."""
    
    files_found = pyqtSignal(list)
    
    def __init__(self, viewer_widget, parent=None, **kwargs):
        super().__init__(parent)
        self.viewer = viewer_widget
        self.train_dir: Optional[Path] = None
        self.is_active = False
        self._timer = QTimer(self)
        self._timer.timeout.connect(self._poll)
        self._last_state = {} # path -> size
        
        self.audit = None
        if parent and hasattr(parent, 'audit'):
            self.audit = parent.audit
    
    def _log(self, message: str, level: str = "ERROR"):
        if self.audit:
            self.audit.add(message, level=level)

    def start(self, train_dir: Path):
        self.train_dir = Path(train_dir)
        self.is_active = True
        self._last_state.clear()
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
            found = self.find_files()
            current_state = {f: f.stat().st_size for f in found if f.exists()}
            
            # Implementation of "Stable File" logic:
            # We only notify when files have grown AND stayed the same size for 1 poll interval.
            # This prevents trying to read a half-written .mha file.
            stable_files = []
            for f in found:
                curr_size = current_state.get(f, 0)
                last_size = self._last_state.get(f, -1)
                
                # If name is same and size is stable (> 0), it's safe to try reading
                if curr_size > 0 and curr_size == last_size:
                    stable_files.append(f)
            
            # If the set of files OR their sizes changed, we update our state.
            # Decouple stability check from state-change check.
            # This ensures that already-stable files (e.g. from a finished run) 
            # are loaded on the second poll.
            self._last_state = current_state
            
            # Use sorted tuple to check for meaningful changes in the stable file list
            stable_tuple = tuple(sorted(stable_files))
            if getattr(self, "_last_stable_tuple", None) != stable_tuple:
                self._last_stable_tuple = stable_tuple
                if stable_files:
                    files_list = sorted(stable_files)
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

    # Coalesce rapid CT_*.mha appearances (parallel DICOM2MHA) so we do not call
    # read_mha_volume + set_mha_volume on the GUI thread for every new file.
    _CT_VIEWER_DEBOUNCE_MS = 550

    def __init__(self, viewer_widget, parent=None, dataset_type: str = "clinical"):
        super().__init__(viewer_widget, parent)
        self.dataset_type = dataset_type
        self._phase_connected = False
        self._all_files = []
        self._last_overlays: Dict[str, str] = {}
        # Cache stores (vol, spacing, origin, direction, sitk_image)
        self._volume_cache: Dict[Path, tuple] = {}
        self._ct_debounce = QTimer(self)
        self._ct_debounce.setSingleShot(True)
        self._ct_debounce.timeout.connect(self._apply_ct_viewer_update)
        self._ct_pending_paths: Optional[Set[Path]] = None

    def start(self, train_dir: Path):
        self._volume_cache.clear()
        self._ct_pending_paths = None
        self._ct_debounce.stop()
        super().start(train_dir)

    def stop(self):
        self._ct_debounce.stop()
        super().stop()

    def get_poll_interval_ms(self) -> int:
        return 200

    def _apply_ct_viewer_update(self):
        if not self._ct_pending_paths:
            return
        paths = sorted(self._ct_pending_paths)
        self._ct_pending_paths = None
        try:
            self.update_viewer(paths)
            self.files_found.emit(paths)
        except Exception as e:
            self._log(f"CTViewerMonitor debounced update error: {e}")

    def _poll(self):
        if not self.is_active or not self.train_dir or not self.train_dir.exists():
            return

        try:
            found = self.find_files()
            current_state = {f: f.stat().st_size for f in found if f.exists()}
            
            # Stability check for SPARE/MHA
            stable_found = set()
            for f in found:
                curr_size = current_state.get(f, 0)
                last_size = self._last_state.get(f, -1)
                
                # In clinical DICOM, we don't strictly need stability, but it doesn't hurt.
                # In SPARE, it's critical.
                if curr_size > 0 and curr_size == last_size:
                    stable_found.add(f)

            if current_state != self._last_state:
                self._last_state = current_state
                # Only debounce update if we have stable files to show
                if stable_found:
                    self._ct_pending_paths = stable_found
                    self._ct_debounce.stop()
                    self._ct_debounce.start(self._CT_VIEWER_DEBOUNCE_MS)

            current_overlays = self._scan_overlays()
            if current_overlays != self._last_overlays:
                self._last_overlays = current_overlays
                self.viewer.set_overlay_sources(current_overlays)

        except Exception as e:
            self._log(f"CTViewerMonitor poll error: {e}")
    
    def prepare_viewer(self):
        try:
            # For SPARE, if we already have a volume (from COPY preview), don't clear it.
            if self.dataset_type == "spare" and hasattr(self.viewer, "is_volume_loaded") and self.viewer.is_volume_loaded():
                return
                
            self.viewer.prepare_for_volume_loading("Awaiting CT volumes...")
            from .ui.info_panels import CTInfoWidget

            if hasattr(self.viewer, "info_panel") and isinstance(
                self.viewer.info_panel, CTInfoWidget
            ):
                # Show Phase / Overlay / Flip as soon as we monitor CT (not COPY DICOM preview).
                self.viewer.info_panel.set_preview_mode(False)
        except Exception:
            pass
    
    def find_files(self) -> list[Path]:
        if not self.train_dir or not self.train_dir.exists():
            return []
        if self.dataset_type == "spare":
            # In SPARE, look for CTs first, then any MHA
            cts = list(self.train_dir.glob("CT_*.mha"))
            if cts:
                return cts
            return list(self.train_dir.glob("*.mha"))
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

            # Always populate Phase (including single CT_*.mha); previously only len>1 ran this,
            # which left an empty combo and no usable phase selection.
            names = [f.name for f in self._all_files]
            self.viewer.info_panel.cmb_phase.blockSignals(True)
            self.viewer.set_phase_options(names)
            self.viewer.info_panel.cmb_phase.setEnabled(True)
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

            # Check cache - invalidate if file size changed
            file_size = file.stat().st_size
            cached_entry = self._volume_cache.get(file)
            
            should_reload = False
            if not cached_entry:
                should_reload = True
            else:
                # Entry 5 is the file size at time of caching
                cached_size = cached_entry[5] if len(cached_entry) > 5 else 0
                if file_size != cached_size:
                    should_reload = True
            
            if should_reload:
                res = read_mha_volume(file)
                if res is None:
                    return # Still stabilizing
                vol, spacing, origin, direction, sitk_img = res
                self._volume_cache[file] = (vol, spacing, origin, direction, sitk_img, file_size)
            else:
                vol, spacing, origin, direction, sitk_img, _ = self._volume_cache[file]
            
            # FIX: Pass sitk_image for geometry-accurate overlay resampling
            self.viewer.set_mha_volume(vol, spacing, origin, direction, sitk_image=sitk_img, dataset_type=self.dataset_type)
            
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
    """Monitors for DRR .tif/.hnc/.bin files and updates DRRViewerPanel live as generation runs."""
    frame_ready = pyqtSignal(int, int, str, object, float)

    # Cap work per timer tick so COMPRESS (tif/png → .bin) cannot enqueue thousands of
    # disk reads + pixmaps on the GUI thread in one shot (extra tabs keep monitors alive).
    _MAX_PROJECTIONS_PER_POLL = 28

    def __init__(self, viewer_widget, parent=None, **kwargs):
        super().__init__(viewer_widget, parent=parent, **kwargs)
        self.frame_ready.connect(self.viewer.add_frame)
        self._drr_paths_ok: Set[Path] = set()

    def start(self, train_dir: Path):
        self._drr_paths_ok.clear()
        super().start(train_dir)

    def get_poll_interval_ms(self) -> int:
        return 150  # Fast poll for live DRR display as projections appear

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
        # PNG first: uint8 Joseph DRRs (typically 768×1024; see generate._png_frame_layout)
        files = list(self.train_dir.glob("*_Proj_*.png"))
        files += list(self.train_dir.glob("*_Proj_*.tif")) + list(self.train_dir.glob("*_Proj_*.tiff"))
        files += list(self.train_dir.glob("*_Proj_*.hnc")) + list(self.train_dir.glob("*_Proj_*.bin"))
        return files
    
    def _poll(self):
        if not self.is_active or not self.train_dir or not self.train_dir.exists():
            return

        try:
            current_files = set(self.find_files())
            self._drr_paths_ok &= current_files
            pending = sorted(current_files - self._drr_paths_ok, key=lambda p: p.name)
            if not pending:
                return
            batch = pending[: self._MAX_PROJECTIONS_PER_POLL]
            done = self.update_viewer(batch)
            self._drr_paths_ok.update(done)
        except Exception as e:
            self._log(f"DRRViewerMonitor poll error: {e}")

    def update_viewer(self, new_files: list[Path]) -> list[Path]:
        import re

        processed: list[Path] = []
        for hnc_path in new_files:
            if not self.is_active:
                break

            match = re.match(r"(\d+)_Proj_(\d+)\.(hnc|bin|tif|tiff|png)", hnc_path.name, re.IGNORECASE)
            if not match:
                continue

            ct_num = int(match.group(1))
            proj_num = int(match.group(2))
            ct_name = f"CT_{ct_num:02d}"

            if hnc_path.suffix.lower() == ".png":
                img_array, angle = self._read_png_drr(hnc_path, proj_num)
                if img_array is None:
                    continue
                pixmap = self._convert_display_uint8_to_pixmap(img_array)
            elif hnc_path.suffix.lower() in (".tif", ".tiff"):
                img_array, angle = self._read_tif_file(hnc_path, proj_num)
                if img_array is None:
                    continue
                pixmap = self._convert_to_pixmap(img_array)
            elif hnc_path.suffix == ".bin":
                img_array, angle = self._read_bin_file(hnc_path)
                angle = angle or 0.0
                if img_array is None:
                    continue
                pixmap = self._convert_to_pixmap(img_array)
            else:
                img_array, angle = self._read_hnc_file(hnc_path)
                if img_array is None:
                    continue
                pixmap = self._convert_to_pixmap(img_array)

            if pixmap and not pixmap.isNull():
                self.frame_ready.emit(ct_num, proj_num - 1, ct_name, pixmap, angle)
                processed.append(hnc_path)
        return processed

    def _read_png_drr(self, png_path: Path, proj_num: int) -> tuple:
        """Load uint8 grayscale PNG from disk (bone=white). No log remap."""
        try:
            import numpy as np
            n_proj = get_projections_per_ct()
            angle = (proj_num - 1) * 360.0 / n_proj if n_proj > 0 else 0.0
            from PIL import Image
            arr = np.array(Image.open(png_path).convert("L"), dtype=np.float32)
            return arr, angle
        except Exception as e:
            self._log(f"Error reading PNG {png_path.name}: {e}", level="WARN")
            return None, None

    def _convert_display_uint8_to_pixmap(self, img_array) -> object:
        """Map PNG uint8 to QPixmap (see _prepare_png_uint8_for_display)."""
        try:
            u8 = np.clip(img_array, 0, 255).astype(np.uint8)
            u8 = _prepare_png_uint8_for_display(u8)
            return _u8_to_grayscale_pixmap(u8)
        except Exception as e:
            self._log(f"Error converting PNG DRR to pixmap: {e}")
            return None

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

    def _read_tif_file(self, tif_path: Path, proj_num: int) -> tuple:
        """Read .tif from Python DRR (uint16, 65535*exp(-atten)). Convert to log for display."""
        try:
            import numpy as np
            n_proj = get_projections_per_ct()
            angle = (proj_num - 1) * 360.0 / n_proj if n_proj > 0 else 0.0
            try:
                import SimpleITK as sitk
                img = sitk.ReadImage(str(tif_path))
                arr = sitk.GetArrayFromImage(img)
            except Exception:
                from PIL import Image
                arr = np.array(Image.open(tif_path).convert("L"))
            if arr.ndim == 3:
                arr = arr[0]
            # Convert uint16 to log space: P = log(65536/(P+1))
            arr = np.log(65536.0 / (arr.astype(np.float32) + 1.0))
            return arr, angle
        except Exception as e:
            self._log(f"Error reading TIF {tif_path.name}: {e}", level="WARN")
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
            import os
            from PyQt6.QtGui import QImage, QPixmap
            wl, ww = 2.5, 3.0
            lo, hi = wl - ww / 2.0, wl + ww / 2.0
            windowed = np.clip(img_array, lo, hi)
            windowed = ((windowed - lo) / max(ww, 1e-6)) * 255.0
            u8_img = np.clip(windowed, 0, 255).astype(np.uint8)
            
            # Default: no transpose (Ribcage-to-spine vertical)
            if os.environ.get("LEARNGUI_DRR_VIEW_TRANSPOSE", "0").strip().lower() in ("1", "true", "yes", "on"):
                u8_img = np.transpose(u8_img)
            
            if not u8_img.flags["C_CONTIGUOUS"]:
                u8_img = np.ascontiguousarray(u8_img)
            return _u8_to_grayscale_pixmap(u8_img)
        except Exception as e:
            self._log(f"Error converting DRR frame to pixmap: {e}")
            return None


class DVFViewerMonitor(ViewerMonitor):
    """Monitors for DVF files and updates DVFPanel."""

    _DVF_VIEWER_DEBOUNCE_MS = 650

    def __init__(self, viewer_widget, mode: str = 'low', parent=None, **kwargs):
        super().__init__(viewer_widget, parent=parent, **kwargs)
        self.mode = mode
        self._source_mha_path: Optional[Path] = None
        self._dvf_debounce = QTimer(self)
        self._dvf_debounce.setSingleShot(True)
        self._dvf_debounce.timeout.connect(self._apply_dvf_viewer_update)
        self._dvf_pending_files: Optional[Set[Path]] = None

    def _apply_dvf_viewer_update(self):
        if not self._dvf_pending_files:
            return
        files = sorted(self._dvf_pending_files)
        self._dvf_pending_files = None
        try:
            self.update_viewer(files)
            self.files_found.emit(files)
        except Exception as e:
            self._log(f"DVFViewerMonitor debounced update error: {e}")

    def stop(self):
        self._dvf_debounce.stop()
        super().stop()

    def _poll(self):
        if not self.is_active or not self.train_dir or not self.train_dir.exists():
            return
        try:
            found = self.find_files()
            current_state = {f: f.stat().st_size for f in found if f.exists()}
            
            stable_found = set()
            for f in found:
                curr_size = current_state.get(f, 0)
                last_size = self._last_state.get(f, -1)
                if curr_size > 0 and curr_size == last_size:
                    stable_found.add(f)

            # Decouple stability check from state-change check.
            self._last_state = current_state
            
            stable_tuple = tuple(sorted(stable_found))
            if getattr(self, "_last_stable_tuple", None) != stable_tuple:
                self._last_stable_tuple = stable_tuple
                if stable_found:
                    self._dvf_pending_files = stable_found
                    self._dvf_debounce.stop()
                    self._dvf_debounce.start(self._DVF_VIEWER_DEBOUNCE_MS)
        except Exception as e:
            self._log(f"DVFViewerMonitor poll error: {e}")

    def start(self, train_dir: Path):
        from .preview import read_mha_volume

        self._dvf_debounce.stop()
        self._dvf_pending_files = None
        super().start(train_dir)
        
        if self.mode == 'low':
            self._source_mha_path = self.train_dir / "sub_CT_06.mha"
            if not self._source_mha_path.exists():
                self._source_mha_path = self.train_dir / "source.mha"
            if not self._source_mha_path.exists():
                # Flexible fallback for SPARE or custom naming
                all_subs = sorted(list(self.train_dir.glob("sub_*.mha")))
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
                # Flexible fallback for SPARE or custom naming
                all_full = sorted(list(self.train_dir.glob("*.mha")))
                # Filter out masks if possible
                mhas = [f for f in all_full if "Mask" not in f.name and "GTVol" not in f.name]
                if mhas:
                    self._source_mha_path = mhas[0]
                elif all_full:
                    # Fallback to absolute first if no CT name found
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
            # Worker outputs DVF_sub_01.mha, DVF_sub_02.mha, ...
            return list(self.train_dir.glob("DVF_sub_*.mha"))
        else:
            # Worker outputs DVF_01.mha, DVF_02.mha, ... (exclude DVF_sub_*)
            return [f for f in self.train_dir.glob("DVF_*.mha") if "sub_" not in f.name]

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
    if parent and hasattr(parent, 'dataset_type'):
        kwargs['dataset_type'] = parent.dataset_type

    if monitor_class == DVFViewerMonitor:
        if parent and hasattr(parent, '_active_step_name'):
            if parent._active_step_name == 'DVF3D_FULL':
                kwargs['mode'] = 'full'
            else:
                kwargs['mode'] = 'low'
    
    return monitor_class(viewer_widget, parent=parent, **kwargs)