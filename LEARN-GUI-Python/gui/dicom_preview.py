# dicom_preview.py
"""
Efficient DICOM preview engine for the COPY step.

Design principles:
- Incremental file discovery (no full rescans)
- Lazy slice reading with caching
- Steady-rate frame emission via queue
- Incremental volume building for MPR views
"""
from __future__ import annotations
from pathlib import Path
from typing import Optional, Dict, List, TYPE_CHECKING, Any, Union
from collections import deque, defaultdict
import numpy as np
import time

from PyQt6.QtCore import QObject, QTimer, pyqtSignal
from PyQt6.QtGui import QPixmap, QImage

if TYPE_CHECKING:
    from pydicom.dataset import Dataset

try:
    import pydicom
    from pydicom.misc import is_dicom
except ImportError:
    pydicom = None
    def is_dicom(_): return False
except Exception:
    pydicom = None
    def is_dicom(_): return False

try:
    import SimpleITK as sitk
except ImportError:
    sitk = None

try:
    from scipy.ndimage import zoom
except ImportError:
    zoom = None


# ============================================================================
# WINDOWING UTILITIES
# ============================================================================

def _window_to_u8(arr: np.ndarray, wl: float, ww: float) -> np.ndarray:
    """Apply window/level and convert to uint8."""
    lo, hi = wl - ww / 2.0, wl + ww / 2.0
    clipped = np.clip(arr, lo, hi)
    scaled = ((clipped - lo) / max(hi - lo, 1e-6)) * 255.0
    return scaled.astype(np.uint8)


def _auto_window(arr: np.ndarray) -> tuple[float, float]:
    """Calculate window/level from percentiles."""
    finite = arr[np.isfinite(arr)]
    if finite.size < 10 or np.ptp(finite) < 1e-3:
        # Fallback for empty or uniform images
        val = float(np.median(finite)) if finite.size > 0 else 0.0
        return val, 100.0
    p5, p95 = np.percentile(finite, [5, 95])
    return (p5 + p95) / 2.0, max(p95 - p5, 1.0)


def _array_to_pixmap(u8: np.ndarray) -> QPixmap:
    """Convert uint8 2D array to QPixmap."""
    if u8.ndim != 2:
        return QPixmap()
    u8 = np.ascontiguousarray(u8)
    h, w = u8.shape
    qimg = QImage(u8.data, w, h, w, QImage.Format.Format_Grayscale8)
    return QPixmap.fromImage(qimg.copy())


# ============================================================================
# DICOM SLICE READER
# ============================================================================

class DicomSlice:
    """Cached DICOM slice with lazy pixel loading."""
    
    __slots__ = ('path', 'instance_number', 'rows', 'cols', 
                 'pixel_spacing', 'slice_thickness', '_pixels', 'last_access',
                 'series_uid')
    
    def __init__(self, path: Path, ds: Dataset):
        self.path = path
        # Fallback to path name for sort stability if InstanceNumber is missing
        self.instance_number = int(getattr(ds, 'InstanceNumber', 0) or 0)
        self.rows = int(getattr(ds, 'Rows', 0) or 0)
        self.cols = int(getattr(ds, 'Columns', 0) or 0)
        
        # Series Grouping
        self.series_uid = str(getattr(ds, "SeriesInstanceUID", "Unknown"))
        
        ps = getattr(ds, 'PixelSpacing', None)
        self.pixel_spacing = (float(ps[0]), float(ps[1])) if ps and len(ps) >= 2 else None
        self.slice_thickness = float(getattr(ds, 'SliceThickness', 0) or 0)
        
        self._pixels: Optional[np.ndarray] = None
        self.last_access = 0.0
    
    def get_pixels(self) -> np.ndarray:
        """Load pixel data on demand (cached after first load)."""
        self.last_access = time.time()
        if self._pixels is None:
            if pydicom is None:
                return np.zeros((self.rows, self.cols), dtype=np.float32)

            ds = pydicom.dcmread(str(self.path), force=True)
            arr = ds.pixel_array.astype(np.float32)
            slope = float(getattr(ds, 'RescaleSlope', 1.0) or 1.0)
            intercept = float(getattr(ds, 'RescaleIntercept', 0.0) or 0.0)
            self._pixels = arr * slope + intercept
        return self._pixels
    
    def clear_cache(self):
        """Release pixel memory."""
        self._pixels = None
    
    @property
    def is_loaded(self) -> bool:
        return self._pixels is not None


class MHAVolume:
    """Previewer for MHA volumes (generates 3-plane MPR view).
    
    Handles partially-written files during COPY by checking file size stability
    before reading, and allows retries on subsequent frames.
    """
    __slots__ = ('path', 'instance_number', 'rows', 'cols', 
                 'series_uid', '_pixels', 'last_access', 'is_mha',
                 '_read_attempts', '_last_file_size')
    
    MAX_READ_ATTEMPTS = 10  # Stop retrying after this many failures
    
    def __init__(self, path: Path):
        self.path = path
        self.instance_number = 0
        self.series_uid = path.name  # Use filename as series UID for distinct selection
        self.rows = 0
        self.cols = 0
        self._pixels = None
        self.last_access = 0.0
        self.is_mha = True
        self._read_attempts = 0
        self._last_file_size = -1
    
    def _is_file_stable(self) -> bool:
        """Check if the file has stopped growing (i.e., copy is complete)."""
        try:
            size = self.path.stat().st_size
            if size == 0:
                return False
            if size != self._last_file_size:
                self._last_file_size = size
                return False  # Size changed since last check — still being written
            return True  # Same size as last check
        except (OSError, FileNotFoundError):
            return False
    
    def get_pixels(self) -> np.ndarray:
        self.last_access = time.time()
        if self._pixels is not None:
            return self._pixels
        
        if sitk is None:
            return np.zeros((1, 1), dtype=np.float32)
        
        # Don't attempt if we've exhausted retries
        if self._read_attempts >= self.MAX_READ_ATTEMPTS:
            return np.zeros((1, 1), dtype=np.float32)
        
        # Check file is stable (not still being written from COPY)
        if not self._is_file_stable():
            return np.zeros((1, 1), dtype=np.float32)
        
        self._read_attempts += 1
        
        try:
            img = sitk.ReadImage(str(self.path))
            arr = sitk.GetArrayFromImage(img)  # (Z, Y, X)
            
            if arr.ndim != 3:
                self._pixels = arr.astype(np.float32)
            else:
                z, y, x = arr.shape
                axial    = arr[z // 2, :, :].astype(np.float32)   # (Y, X)
                coronal  = arr[:, y // 2, :].astype(np.float32)   # (Z, X)
                sagittal = arr[:, :, x // 2].astype(np.float32)   # (Z, Y)
                
                # Pad all planes to the same height (max of Y, Z)
                target_h = max(y, z)
                
                def _pad_to_h(plane, target):
                    h, w = plane.shape
                    if h >= target:
                        return plane
                    pad_top = (target - h) // 2
                    pad_bot = target - h - pad_top
                    return np.pad(plane, ((pad_top, pad_bot), (0, 0)),
                                  mode='constant', constant_values=float(plane.min()))
                
                ax_p = _pad_to_h(axial, target_h)
                co_p = _pad_to_h(coronal, target_h)
                sa_p = _pad_to_h(sagittal, target_h)
                
                # 2px dark separator columns between views
                sep = np.full((target_h, 2), float(ax_p.min()), dtype=np.float32)
                self._pixels = np.hstack([ax_p, sep, co_p, sep, sa_p])
            
            self.rows, self.cols = self._pixels.shape
            return self._pixels
            
        except Exception:
            # File may still be incomplete (copy in progress) — allow retry
            # by NOT setting self._pixels
            return np.zeros((1, 1), dtype=np.float32)

    def clear_cache(self):
        self._pixels = None
        self._read_attempts = 0  # Reset retries on cache clear
        self._last_file_size = -1

    @property
    def is_loaded(self) -> bool:
        return self._pixels is not None


# ============================================================================
# DICOM PREVIEW ENGINE
# ============================================================================

class DICOMPreviewEngine(QObject):
    """
    Efficient DICOM preview with:
    - Incremental file discovery
    - Smooth carousel playback (sorted by InstanceNumber)
    - Automatic Series Detection (only shows the largest series)
    - Time-budgeted header parsing
    """
    
    # Signals
    frame_ready = pyqtSignal(object, object, object, dict)  # ax, co, sa pixmaps + info
    discovery_update = pyqtSignal(int)  # total files found
    series_catalog_changed = pyqtSignal(list)  # list[tuple[str, str]] (label, series_uid)
    
    # Default CT window
    DEFAULT_WL, DEFAULT_WW = -600.0, 1500.0
    
    def __init__(self, train_dir: Path, 
                 display_fps: int = 15,
                 discovery_interval_ms: int = 200,
                 enable_mpr: bool = False,
                 logger: Optional[callable] = None,
                 parent=None):
        super().__init__(parent)
        
        self.train_dir = Path(train_dir)
        self.display_fps = max(1, display_fps)
        self.discovery_interval_ms = max(50, discovery_interval_ms)
        self.enable_mpr = enable_mpr
        self._logger = logger
        self._max_cached_slices = 50  # Limit memory usage
        
        # File tracking
        self._known_paths: set[Path] = set()
        self._processing_queue: deque[Path] = deque() 
        self._retry_counts: Dict[Path, int] = {}
        self._slices: Dict[Path, Any] = {}
        
        # Series grouping
        self._series_map: Dict[str, List[Any]] = defaultdict(list)
        self._active_uid: Optional[str] = None
        self._manual_series_lock = False  # True after user picks a series in the COPY helper
        self._catalog_timer = QTimer(self)
        self._catalog_timer.setSingleShot(True)
        self._catalog_timer.timeout.connect(self._emit_series_catalog)
        
        # Sorting
        self._sorted_slices: List[Any] = []
        self._sorted_valid = True
        self._volume_valid = False
        
        # Volume for MPR
        self._volume: Optional[np.ndarray] = None
        self._volume_valid = False
        self._last_volume_count = 0
        
        # Display state
        self._display_index = 0
        self._wl, self._ww = self.DEFAULT_WL, self.DEFAULT_WW
        self._auto_windowed = False
        
        # Timers
        self._discovery_timer = QTimer(self)
        self._discovery_timer.timeout.connect(self._discover_files)
        
        self._display_timer = QTimer(self)
        self._display_timer.timeout.connect(self._emit_next_frame)
        
        self._is_running = False
    
    def start(self):
        """Start discovery and display."""
        if self._is_running:
            return
        self._is_running = True
        
        # Initial discovery
        self._discover_files()
        
        # Start timers
        self._discovery_timer.start(self.discovery_interval_ms)
        self._display_timer.start(1000 // self.display_fps)
    
    def stop(self):
        """Stop all activity."""
        self._is_running = False
        self._catalog_timer.stop()
        self._discovery_timer.stop()
        self._display_timer.stop()
        
        # Clear caches to free memory
        for sl in self._slices.values():
            sl.clear_cache()
        self._slices.clear()
        self._volume = None
        self._manual_series_lock = False

    def _effective_active_uid(self) -> Optional[str]:
        if self._manual_series_lock and self._active_uid and self._active_uid in self._series_map:
            return self._active_uid
        if not self._series_map:
            return None
        return max(self._series_map.keys(), key=lambda u: len(self._series_map[u]))

    def set_active_series_uid(self, uid: Optional[str]) -> None:
        """'' or None = auto (largest series); otherwise lock preview to that SeriesInstanceUID."""
        u = (uid or "").strip()
        if not u:
            self._manual_series_lock = False
            self._active_uid = None
        else:
            self._manual_series_lock = True
            self._active_uid = u if u in self._series_map else None
        self._sorted_valid = False
        self._display_index = 0

    def _emit_series_catalog(self) -> None:
        if not self._is_running:
            return
        opts: List[tuple[str, str]] = []
        for uid, slices in sorted(self._series_map.items(), key=lambda kv: -len(kv[1])):
            n = len(slices)
            # Check if this is an MHA volume (uid == filename)
            is_mha = any(getattr(s, 'is_mha', False) for s in slices)
            if is_mha:
                opts.append((f"Volume · {uid}", uid))
            else:
                short = f"{uid[:10]}…" if len(uid) > 12 else uid
                opts.append((f"{n} slices · {short}", uid))
        self.series_catalog_changed.emit(opts)

    def _schedule_emit_catalog(self) -> None:
        if self._is_running:
            self._catalog_timer.start(150)

    # ========================================================================
    # FILE DISCOVERY (lightweight)
    # ========================================================================
    
    def _discover_files(self):
        """Find new DICOM files without reading headers."""
        if not self.train_dir.exists():
            return
        
        # Fast glob for common DICOM extensions
        new_files = []
        # Check ALL files if there aren't too many, otherwise use patterns
        try:
            for p in self.train_dir.rglob("*"):
                if not p.is_file() or p in self._known_paths:
                    continue
                    
                ext = p.suffix.lower()
                is_valid = False
                
                if ext in {".dcm", ".dicom", ".ima"}:
                    is_valid = True
                elif ext == ".mha":
                    is_valid = True
                elif ext == "" or ext.isdigit(): # Handle extensionless or numeric extensions
                    try:
                        with open(p, 'rb') as f:
                            f.seek(128)
                            if f.read(4) == b'DICM':
                                is_valid = True
                    except Exception:
                        pass
                
                if is_valid:
                    new_files.append(p)
        except Exception as e:
            if self._logger: self._logger(f"DEBUG: Discovery error: {e}")
        
        if new_files:
            self._known_paths.update(new_files)
            # Add to processing queue
            self._processing_queue.extend(sorted(new_files))
            self.discovery_update.emit(len(self._known_paths))
    
    # ========================================================================
    # SLICE MANAGEMENT
    # ========================================================================
    
    def _ensure_slice(self, path: Path) -> Optional[Union[DicomSlice, MHAVolume]]:
        """Get or create DicomSlice or MHAVolume (reads header/first load)."""
        if path in self._slices:
            return self._slices[path]
        
        try:
            if path.suffix.lower() == ".mha":
                sl = MHAVolume(path)
                # To get rows/cols, we need to load once or just wait
                # MHASlice is lazy, let's just use it
            elif pydicom is not None:
                ds = pydicom.dcmread(str(path), stop_before_pixels=True, force=True)
                
                # Verify it's CT or fallback if missing
                modality = str(getattr(ds, 'Modality', 'CT')).upper()
                if modality not in {'CT', 'MR', 'RTDOSE', 'RTSTRUCT', 'RTPLAN', 'PT'}:
                    # If it's a known non-image type or completely wrong, skip
                    if any(x in modality for x in {'REPORT', 'SR', 'KO', 'PR'}):
                        return None
                
                sl = DicomSlice(path, ds)
            else:
                return None

            self._slices[path] = sl
            
            # Add to series bucket
            self._series_map[sl.series_uid].append(sl)
            
            # Auto-pick largest series while user has not locked a choice
            if not self._manual_series_lock:
                if self._active_uid is None:
                    self._active_uid = sl.series_uid
                elif self._active_uid in self._series_map:
                    current_len = len(self._series_map[self._active_uid])
                    new_len = len(self._series_map[sl.series_uid])
                    if new_len > current_len:
                        self._active_uid = sl.series_uid
            
            self._sorted_valid = False # Mark sort as dirty
            self._schedule_emit_catalog()
            
            # Memory management
            self._cleanup_old_caches()
            
            return sl
            
        except Exception as e:
            # Race condition fix: If read fails (likely file still being written),
            # don't discard it. Instead, we'll let it stay in _known_paths
            # but NOT in _slices, and it might be re-processed if we re-add to queue.
            # For now, let's just log it.
            if self._logger: self._logger(f"DEBUG: Failed to read header for {path.name}: {e}")
            return None
    
    def _cleanup_old_caches(self):
        """Clear pixel caches from least recently accessed slices."""
        loaded = [s for s in self._slices.values() if s.is_loaded]
        if len(loaded) <= self._max_cached_slices:
            return
        
        # Sort by last access, clear oldest
        loaded.sort(key=lambda s: s.last_access)
        to_clear = len(loaded) - self._max_cached_slices
        for sl in loaded[:to_clear]:
            sl.clear_cache()
    
    def _get_sorted_slices(self) -> List[Any]:
        """Get slices sorted by instance number for the ACTIVE series only."""
        if not self._sorted_valid:
            eff = self._effective_active_uid()
            if eff and eff in self._series_map:
                active_group = self._series_map[eff]
                self._sorted_slices = sorted(
                    active_group,
                    key=lambda s: (s.instance_number, s.path.name)
                )
            else:
                self._sorted_slices = []
                
            self._sorted_valid = True
            self._volume_valid = False # Invalidate MPR volume when list changes
            
        return self._sorted_slices
    
    # ========================================================================
    # VOLUME BUILDING (incremental)
    # ========================================================================
    
    def _rebuild_volume_if_needed(self) -> bool:
        """Rebuild volume only if new slices added. Returns True if valid."""
        slices = self._get_sorted_slices()
        
        if len(slices) < 3:
            return False
        
        # Only rebuild if slice count changed significantly
        if self._volume_valid and len(slices) == self._last_volume_count:
            return True
        
        # Check if we have enough new slices to justify rebuild
        if self._volume is not None:
            new_count = len(slices) - self._last_volume_count
            if new_count < 5 and new_count < len(slices) * 0.1:
                return self._volume is not None
        
        try:
            # Stack all slices
            arrays = []
            for sl in slices:
                arr = sl.get_pixels()
                if arr is not None:
                    arrays.append(arr)
            
            if len(arrays) >= 3:
                self._volume = np.stack(arrays, axis=0)
                self._volume_valid = True
                self._last_volume_count = len(slices)
                
                if not self._auto_windowed:
                    self._wl, self._ww = _auto_window(self._volume)
                    self._auto_windowed = True
                
                return True
                
        except Exception:
            pass
        
        return False
    
    # ========================================================================
    # FRAME EMISSION
    # ========================================================================
    
    def _emit_next_frame(self):
        """Emit next frame from SORTED list (Smooth Carousel)."""
        if not self._is_running:
            return
        
        # 1. Ingest Phase: Process headers from the queue
        start_time = time.time()
        TIME_BUDGET = 0.05
        
        while self._processing_queue:
            if time.time() - start_time > TIME_BUDGET:
                break 
            path = self._processing_queue.popleft()
            if not self._ensure_slice(path):
                # If it failed to read (maybe incomplete file), put it back at the end of the queue
                # but only if we haven't tried too many times for this specific file.
                # To keep it simple, just put it back once.
                if hasattr(self, '_retry_counts'):
                    count = self._retry_counts.get(path, 0)
                    if count < 5:
                        self._retry_counts[path] = count + 1
                        self._processing_queue.append(path)
                else:
                    self._retry_counts = {path: 1}
                    self._processing_queue.append(path)
        
        # 2. Sort Phase: Get the ordered playlist for the ACTIVE series
        slices = self._get_sorted_slices()
        if not slices:
            if not getattr(self, '_no_slices_logged', False):
                if self._logger: self._logger("DEBUG: Preview engine started but no slices found in active series yet.")
                self._no_slices_logged = True
            return

        self._no_slices_logged = False

        # 3. Display Phase: Increment carousel index
        self._display_index = (self._display_index + 1) % len(slices)
        sl = slices[self._display_index]
        
        # 4. Render Phase
        try:
            pixels = sl.get_pixels()
            if pixels is None:
                return
            
            # Skip rendering 1×1 fallback (incomplete MHA, still being copied)
            if pixels.shape[0] <= 1 or pixels.shape[1] <= 1:
                return
            
            if not self._auto_windowed:
                self._wl, self._ww = _auto_window(pixels)
                self._auto_windowed = True
            
            ax_u8 = _window_to_u8(pixels, self._wl, self._ww)
            ax_pm = _array_to_pixmap(ax_u8)
            
            # MPR (Optional)
            co_pm, sa_pm = None, None
            if self.enable_mpr and self._rebuild_volume_if_needed() and self._volume is not None:
                # Ensure _volume is actually there before indexing
                vol = self._volume
                z, y, x = vol.shape
                try:
                    idx = self._sorted_slices.index(sl)
                except ValueError:
                    idx = z // 2
                
                co_slice = vol[:, y // 2, :]
                co_u8 = _window_to_u8(co_slice, self._wl, self._ww)
                co_pm = _array_to_pixmap(co_u8)
                
                sa_slice = vol[:, :, x // 2]
                sa_u8 = _window_to_u8(sa_slice, self._wl, self._ww)
                sa_pm = _array_to_pixmap(sa_u8)
            
            # Info
            is_mha = getattr(sl, 'is_mha', False)
            type_label = "MHA" if is_mha else "DICOM"
            
            if is_mha:
                info = {
                    'source': f"{type_label}: {sl.path.name}",
                    'dims': f"{sl.rows} × {sl.cols}",
                    'vox': "—",
                    'count': str(len(slices)),
                }
            else:
                info = {
                    'source': f"{type_label}: {sl.path.name}",
                    'dims': f"{len(slices)} × {sl.rows} × {sl.cols}",
                    'vox': (f"{sl.slice_thickness:.3f}, {sl.pixel_spacing[0]:.3f}, {sl.pixel_spacing[1]:.3f}"
                            if getattr(sl, 'pixel_spacing', None) else "—"),
                    'count': str(len(slices)),
                }
            
            self.frame_ready.emit(ax_pm, co_pm, sa_pm, info)
            
            if not self._first_frame_logged:
                eff = self._effective_active_uid() or ""
                if self._logger and eff:
                    self._logger(f"INFO: DICOM preview active. Showing series {eff[:8]}...")
                self._first_frame_logged = True
            
        except Exception as e:
            if self._logger: self._logger(f"DEBUG: Frame emission error: {e}")
    
    # ========================================================================
    # PUBLIC INTERFACE
    # ========================================================================
    
    def set_window(self, wl: float, ww: float):
        self._wl, self._ww = wl, ww
        self._auto_windowed = True
    
    def get_slice_count(self) -> int:
        return len(self._known_paths)
    
    def get_loaded_count(self) -> int:
        return len(self._slices)