# ui/panels.py
from __future__ import annotations
from pathlib import Path
from PyQt6.QtCore import QSize, Qt, pyqtSignal, QTimer, QPoint, QEvent, QObject
from PyQt6.QtGui import QPainter, QColor, QPixmap, QImage, QIcon, QTransform
from PyQt6.QtWidgets import (
    QGroupBox, QVBoxLayout, QLabel, QGridLayout, QFormLayout,
    QWidget, QSlider, QComboBox, QSizePolicy, QCheckBox, QHBoxLayout,
    QScrollArea, QPushButton, QStyle, QDoubleSpinBox,
)
import numpy as np
import matplotlib
from ..preview import _auto_window, _window_u8, _to_qpixmap, read_mha_volume
from typing import Dict, Optional, Tuple
import traceback
from ..ui.info_panels import CTInfoWidget, DVFInfoWidget, BaseInfoWidget # NEW IMPORT

# --- Overlay color definitions (not red!) ---
OVERLAY_COLORS = {
    "Body": (0, 200, 255),      # Cyan
    "GTV_Inh": (255, 200, 0),   # Orange/Gold
    "GTV_Exh": (255, 150, 0),   # Darker orange
    "Lungs_Inh": (100, 255, 100),  # Light green
    "Lungs_Exh": (50, 200, 50),    # Darker green
}

def apply_placeholder_style(lbl: QLabel, text: str) -> None:
    lbl.setText(text)
    lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
    lbl.setStyleSheet(
        "background-color:#000;"
        "color:#9aa0a6;"
        "border:1px dashed #444;"
        "border-radius:0;"
    )

# =========================================================
# ImagePanel (Unchanged)
# =========================================================
class ImagePanel(QGroupBox):
    def __init__(self, title: str, subtitle: str):
        super().__init__(title)
        v = QVBoxLayout(self)
        self.lbl = QLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self._orig: QPixmap | None = None
        apply_placeholder_style(self.lbl, subtitle)
        v.addWidget(self.lbl)

    def set_pixmap(self, pix: QPixmap):
        if not pix:
            return
        self._orig = pix
        self.lbl.setText("")
        box = self.lbl.size()
        scaled = pix.scaled(
            box,
            Qt.AspectRatioMode.KeepAspectRatio,
            Qt.TransformationMode.SmoothTransformation
        )
        self.lbl.setPixmap(scaled)

    def resizeEvent(self, e):
        if self._orig is not None:
            box = self.lbl.size()
            self.lbl.setPixmap(self._orig.scaled(
                box,
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation
            ))
        super().resizeEvent(e)

# =========================
# Helpers (Unchanged)
# =========================
def _robust_levels(arr: np.ndarray, lo=0.5, hi=99.5) -> tuple[float, float]:
    finite = np.isfinite(arr)
    if not finite.any():
        mn, mx = float(np.min(arr)), float(np.max(arr))
        if mx <= mn: mx = mn + 1.0
        return mn, mx
    p_lo, p_hi = np.percentile(arr[finite], [lo, hi])
    if p_hi <= p_lo: p_hi = p_lo + 1.0
    return float(p_lo), float(p_hi)

def _window_to_u8_levels(im: np.ndarray, lo: float, hi: float) -> np.ndarray:
    x = np.clip((im.astype(np.float32) - lo) / max(1e-6, (hi - lo)), 0.0, 1.0)
    return (x * 255.0 + 0.5).astype(np.uint8)

def _to_qpixmap_rgb(rgb_u8: np.ndarray) -> QPixmap | None:
    try:
        h, w, c = rgb_u8.shape
        if c != 3:
            return None
        arr = np.ascontiguousarray(rgb_u8)
        qimg = QImage(arr.data, w, h, 3 * w, QImage.Format.Format_RGB888)
        return QPixmap.fromImage(qimg.copy())
    except Exception:
        return None

# =========================
# Lightweight label overlays (Unchanged)
# =========================
class OverlayLabel(QLabel):
    """QLabel that draws small overlay texts at top-left, bottom-left, bottom-right."""
    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)
        self._bl = ""
        self._br = ""
        self._tl = ""
    def set_bl_text(self, s: str): self._bl = s or ""; self.update()
    def set_br_text(self, s: str): self._br = s or ""; self.update()
    def set_tl_text(self, s: str): self._tl = s or ""; self.update()
    def paintEvent(self, e):
        super().paintEvent(e)
        if not (self._bl or self._br or self._tl): return
        p = QPainter(self); p.setRenderHint(QPainter.RenderHint.Antialiasing)
        m = 6
        if self._tl:
            p.setPen(QColor(0,0,0,220))
            p.drawText(self.rect().adjusted(7,5,0,0), Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop, self._tl)
            p.setPen(QColor(255,255,255))
            p.drawText(self.rect().adjusted(6,4,0,0), Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop, self._tl)
        if self._bl:
            p.setPen(QColor(0,0,0,220))
            p.drawText(self.rect().adjusted(m+1,0,-m+1,-3), Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignBottom, self._bl)
            p.setPen(QColor(255,255,255))
            p.drawText(self.rect().adjusted(m,0,-m,-4), Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignBottom, self._bl)
        if self._br:
            p.setPen(QColor(0,0,0,220))
            p.drawText(self.rect().adjusted(m+1,0,-m+1,-3), Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignBottom, self._br)
            p.setPen(QColor(255,255,255))
            p.drawText(self.rect().adjusted(m,0,-m,-4), Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignBottom, self._br)
        p.end()


# =========================
# Decentralized Viewer Pane
# =========================

class CTPane(QWidget):
    """
    A single viewer pane containing a control header (dropdown + rotation),
    the image label, and a slice slider.
    """
    settings_changed = pyqtSignal()
    slice_changed = pyqtSignal(int)

    def __init__(self, axis_index: int, default_name="Axial"):
        super().__init__()
        self.layout = QVBoxLayout(self)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self.layout.setSpacing(2)

        # 1. Header Bar
        self.header = QWidget()
        self.header.setStyleSheet("background-color: #222; border-bottom: 1px solid #444;")
        h_lay = QHBoxLayout(self.header)
        h_lay.setContentsMargins(4, 2, 4, 2)
        h_lay.setSpacing(4)

        self.cmb_view = QComboBox()
        self.cmb_view.addItems(["Axial", "Coronal", "Sagittal"])
        self.cmb_view.setCurrentText(default_name)
        # styling to make it small
        self.cmb_view.setStyleSheet("QComboBox { font-size: 10px; padding: 1px; }")
        
        self.btn_ccw = QPushButton("↺")
        self.btn_cw = QPushButton("↻")
        self.btn_flip_h = QPushButton("↔")
        self.btn_flip_v = QPushButton("↕")
        
        for b in (self.btn_ccw, self.btn_cw, self.btn_flip_h, self.btn_flip_v):
            b.setFixedSize(22, 18)
            b.setStyleSheet("font-size: 10px; padding: 0px;")

        h_lay.addWidget(self.cmb_view)
        h_lay.addStretch()
        h_lay.addWidget(self.btn_ccw)
        h_lay.addWidget(self.btn_cw)
        h_lay.addWidget(self.btn_flip_h)
        h_lay.addWidget(self.btn_flip_v)

        # 2. Image Label
        self.lbl = OverlayLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self.lbl.setStyleSheet("background-color:#000; border:0; border-radius:0;")
        self.lbl.setMinimumSize(64, 64)
        self.lbl.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

        # 3. Slider
        self.sld = QSlider(Qt.Orientation.Horizontal)
        self.sld.setEnabled(False)
        self.sld.setMinimum(0)
        self.sld.setMaximum(0)
        self.sld.setStyleSheet("QSlider::handle:horizontal { height: 10px; width: 10px; }")

        self.layout.addWidget(self.header)
        self.layout.addWidget(self.lbl, 1)
        self.layout.addWidget(self.sld)

        # Pane State
        self.axis_index = axis_index
        self.rotation_k = 0  # 0-3
        self.flip_h = False
        self.flip_v = False

        # Connections
        self.cmb_view.currentTextChanged.connect(lambda _: self.settings_changed.emit())
        self.btn_ccw.clicked.connect(self._rotate_ccw)
        self.btn_cw.clicked.connect(self._rotate_cw)
        self.btn_flip_h.clicked.connect(self._toggle_flip_h)
        self.btn_flip_v.clicked.connect(self._toggle_flip_v)
        self.sld.valueChanged.connect(self.slice_changed.emit)

    def _rotate_ccw(self):
        self.rotation_k = (self.rotation_k + 1) % 4
        self.settings_changed.emit()

    def _rotate_cw(self):
        self.rotation_k = (self.rotation_k - 1) % 4
        self.settings_changed.emit()

    def _toggle_flip_h(self):
        self.flip_h = not self.flip_h
        self.settings_changed.emit()

    def _toggle_flip_v(self):
        self.flip_v = not self.flip_v
        self.settings_changed.emit()

    @property
    def view_type(self) -> str:
        return self.cmb_view.currentText()

    def set_view_type(self, vt: str):
        self.cmb_view.setCurrentText(vt)


# =========================
# CT Triptych
# =========================
class CTQuadPanel(QGroupBox):
    """
    CT viewer. 
    Refactored to use CTInfoWidget.
    """
    series_changed = pyqtSignal(int, str)
    slices_changed = pyqtSignal(int, int, int)

    def __init__(self, title: str = "Planning CT", custom_info_widget: BaseInfoWidget = None):
        super().__init__(title)

        self._mode: str = "single"
        self.grid = QGridLayout(self)
        self.grid.setContentsMargins(0, 0, 0, 0)
        self.grid.setHorizontalSpacing(6)
        self.grid.setVerticalSpacing(6)

        # Image panes (Fixed to logical indices 0, 1, 2)
        self.pane_ax = CTPane(axis_index=0, default_name="Axial")
        self.pane_co = CTPane(axis_index=1, default_name="Coronal")
        self.pane_sa = CTPane(axis_index=2, default_name="Sagittal")
        
        self._axis_to_name = {0: "Axial", 1: "Coronal", 2: "Sagittal"}
        
        # Backward compatibility aliases (to minimize diffs in rendering code for now)
        self.lbl_ax = self.pane_ax.lbl
        self.lbl_co = self.pane_co.lbl
        self.lbl_sa = self.pane_sa.lbl
        self.sld_ax = self.pane_ax.sld
        self.sld_co = self.pane_co.sld
        self.sld_sa = self.pane_sa.sld

        # --- Info Panel (Refactored) ---
        if custom_info_widget:
            self.info_panel = custom_info_widget
        else:
            self.info_panel = CTInfoWidget()
            self.info_panel.phase_changed.connect(self.series_changed.emit)
            self.info_panel.overlay_changed.connect(self.set_overlay)
            # Connectivity cleanup: global orientation/rotation removed from UI
            self.info_panel.set_preview_mode(True)

        # Connect Pane signals
        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            p.settings_changed.connect(self._on_pane_settings_changed)
            p.slice_changed.connect(self._on_slice_changed)

        # Layout
        self._apply_layout_single()
        
        # Viewer state
        self.swap_axial_coronal = False
        self._vol_log: np.ndarray | None = None
        self._spacing_log: tuple[float,float,float] | None = None
        self._levels: tuple[float,float] | None = None

        self._cS = self._cA = self._cR = 0
        self._iS = self._iA = self._iR = 0

        self._pm_ax: QPixmap | None = None
        self._pm_co: QPixmap | None = None
        self._pm_sa: QPixmap | None = None

        # Overlay state
        self._overlay_sources: dict[str, str] = {}
        self._overlay_name: str | None = None
        self._overlay_cache: dict[str, np.ndarray] = {}
        self._overlay_tag: tuple | None = None
        self._ov_log: np.ndarray | None = None
        self._overlay_paths: dict[str, str] = {}

        # CT mapping for overlays
        self._sitk_base_img = None
        self._sar_meta = None
        self._sar_reorient_params = None  # Stores perm/flips for overlay processing
        self._dataset_type = "clinical"
        self._perm_phys_to_log: tuple[int,int,int] | None = None
        self._spacing_SAR: tuple[float,float,float] | None = None
        self._vol_SAR: np.ndarray | None = None
        self._orientation_token: str = "S-A-R"
        self._rotate_k: int = 0

    # ---------------- layout helpers ----------------
    def _clear_grid(self):
        while self.grid.count():
            item = self.grid.takeAt(0)
            w = item.widget()
            if w: self.grid.removeWidget(w)

    def _apply_layout_single(self):
        self.pane_co.setVisible(False)
        self.pane_sa.setVisible(False)

        self._clear_grid()
        self.grid.addWidget(self.pane_ax,    0, 0, 2, 1)
        self.grid.addWidget(self.info_panel, 0, 1, 2, 1)
        self.grid.setColumnStretch(0, 3); self.grid.setColumnStretch(1, 2)
        self.grid.setRowStretch(0, 1);    self.grid.setRowStretch(1, 1)
        self.pane_ax.sld.setVisible(False)

    def _apply_layout_triptych(self):
        self.pane_ax.setVisible(True)
        self.pane_co.setVisible(True)
        self.pane_sa.setVisible(True)
        
        self._clear_grid()
        # Usually: Top=Co, BottomLeft=Ax, BottomRight=Sa
        self.grid.addWidget(self.pane_co, 0, 0)
        self.grid.addWidget(self.info_panel, 0, 1)
        self.grid.addWidget(self.pane_ax, 1, 0)
        self.grid.addWidget(self.pane_sa, 1, 1)
        
        self.grid.setColumnStretch(0, 1); self.grid.setColumnStretch(1, 1)
        self.grid.setRowStretch(0, 1);    self.grid.setRowStretch(1, 1)
        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            p.sld.setVisible(True)
        QTimer.singleShot(0, self._rescale_all)

    def set_mode(self, mode: str):
        mode = (mode or "").lower()
        if mode == self._mode or mode not in ("single", "triptych"):
            return
        if self._vol_log is not None and mode == "single":
            return
        self._mode = mode
        if mode == "single":
            self._apply_layout_single()
        else:
            self._apply_layout_triptych()
        self._rescale_all()

    # ---------------- info box API ----------------
    def set_info(self, source: str = "—", dims: str = "—", vox: str = "—", count: str = "—"):
        self.info_panel.set_common_info(source, dims, vox, count)

    def set_info_dict(self, info: dict):
        source_raw = str(info.get("source", "—"))
        # Strip both DICOM: and MHA: prefixes for cleaner display
        for prefix in ("DICOM:", "MHA:"):
            if source_raw.startswith(prefix):
                source_raw = source_raw[len(prefix):].strip()
                break
        self.set_info(
            source=source_raw,
            dims=str(info.get("dims", "—")),
            vox=str(info.get("vox", "—")),
            count=str(info.get("count", "—")),
        )

    def clear(self):
        self._vol_log = None
        self._spacing_log = None
        self._levels = None
        self._pm_ax = self._pm_co = self._pm_sa = None
        self._sitk_base_img = None
        self._overlay_sources.clear()
        self._overlay_cache.clear()
        self._overlay_tag = None
        self._ov_log = None

        self.set_mode("single")
        
        for lbl in (self.lbl_ax, self.lbl_co, self.lbl_sa):
            lbl.setPixmap(QPixmap())
            lbl.setText("") 
        
        apply_placeholder_style(self.lbl_ax, "Volume Preview")
        self.lbl_ax.set_bl_text("Axial")
        self.lbl_co.set_bl_text("Coronal")
        self.lbl_sa.set_bl_text("Sagittal")
        
        self.set_info("—", "—", "—", "—")
        
        # Only clear specific fields if we are using the CT widget
        if isinstance(self.info_panel, CTInfoWidget):
            self.info_panel.update_phase_list([])
            self.info_panel.update_overlay_list([])
            
        for sld in (self.sld_ax, self.sld_co, self.sld_sa):
            sld.setEnabled(False)
            sld.setValue(0)

        self.lbl_ax.repaint()

    def prepare_for_volume_loading(self, message: str = "Awaiting CT volume…"):
        for lbl in (self.lbl_ax, self.lbl_co, self.lbl_sa):
            lbl.setPixmap(QPixmap())
        self.lbl_ax.set_tl_text(message)
        self.lbl_co.set_tl_text(message)
        self.lbl_sa.set_tl_text(message)
        self._rescale_all()

    def set_phase_options(self, names: list[str], current_index: int = 0):
        if isinstance(self.info_panel, CTInfoWidget):
            self.info_panel.update_phase_list(names, current_index)

    # --------------- overlays ---------------
    def set_overlay_sources(self, mapping: dict[str, str]):
        mapping = dict(mapping or {})
        self._overlay_sources = mapping
        self._overlay_paths = mapping
        self._overlay_cache.clear()
        self._overlay_tag = None
        self._ov_log = None
        self._overlay_name = None

        # Update Info Widget
        if isinstance(self.info_panel, CTInfoWidget):
            required_overlays = {"Body", "GTV_Inh", "GTV_Exh", "Lungs_Inh", "Lungs_Exh"}
            all_present = required_overlays.issubset(mapping.keys())
            if all_present:
                self.info_panel.update_overlay_list(list(mapping.keys()))
            else:
                self.info_panel.update_overlay_list([])

    def set_overlay(self, name: str | None):
        self._overlay_name = name or None
        if self._vol_log is None or self._sitk_base_img is None:
            self._ov_log = None
            return
        self._ov_log = self._ensure_overlay_loaded(self._overlay_name)
        self._update_triptych()

    def _ensure_overlay_loaded(self, name: str | None) -> np.ndarray | None:
        """
        Load overlay and transform it to match the CT volume's logical orientation.
        
        Key insight: We must apply the EXACT same transforms to overlay as we did to CT.
        Using stored _sar_reorient_params ensures identical array manipulation.
        """
        if not name:
            return None
        if self._sitk_base_img is None or self._vol_log is None:
            return None
        if not hasattr(self, '_sar_reorient_params') or self._sar_reorient_params is None:
            return None

        # Cache check with geometry tag (includes SAR params to detect orientation changes)
        current_origin = self._sitk_base_img.GetOrigin()
        sar_params = self._sar_reorient_params
        tag = (
            tuple(self._vol_log.shape), 
            tuple(self._perm_phys_to_log), 
            tuple(current_origin),
            sar_params['perm_zyx_to_SAR'],
            sar_params['flips_SAR']
        )
        
        cached = self._overlay_cache.get(name)
        if cached is not None and self._overlay_tag == tag:
            return cached

        paths = self._overlay_sources or self._overlay_paths
        path = paths.get(name)
        if not path:
            return None

        try:
            import os
            import SimpleITK as sitk
            if not os.path.exists(path):
                return None

            ov_img = sitk.ReadImage(path)
            
            # Resample overlay to CT grid (same size/spacing/origin/direction)
            ov_res = sitk.Resample(
                ov_img, 
                self._sitk_base_img,
                sitk.Transform(),
                sitk.sitkNearestNeighbor,
                0.0,
                ov_img.GetPixelIDValue()
            )
            
            # Get array - now on same voxel grid as CT before reorientation
            ov_arr = sitk.GetArrayFromImage(ov_res).astype(np.float32)
            
            # Apply EXACT same reorientation we applied to CT (stored params)
            ov_SAR = self._apply_sar_reorientation(ov_arr)
            
            # Apply same logical permutation
            ov_log = np.transpose(ov_SAR, self._perm_phys_to_log)
            
            if ov_log.dtype != np.uint8:
                ov_log = (ov_log > 0).astype(np.uint8)

            self._overlay_cache[name] = ov_log
            self._overlay_tag = tag
            return ov_log

        except Exception as e:
            print(f"Overlay load error for {name}: {e}")
            import traceback
            traceback.print_exc()
            return None
    
    def _apply_sar_reorientation(self, arr_zyx: np.ndarray) -> np.ndarray:
        """Apply stored SAR reorientation parameters to array."""
        params = self._sar_reorient_params
        perm = params['perm_zyx_to_SAR']
        flips = params['flips_SAR']
        
        vol = np.transpose(arr_zyx, perm)
        if flips[0]: vol = vol[::-1, :, :]
        if flips[1]: vol = vol[:, ::-1, :]
        if flips[2]: vol = vol[:, :, ::-1]
        
        return vol

    # =========================
    # MHA volume API (Logic Unchanged, just referencing self.info_panel)
    # =========================
    def set_mha_volume(self, vol: np.ndarray, spacing: tuple[float,float,float], origin=None, direction=None, sitk_image=None, dataset_type="clinical"):
        self._dataset_type = dataset_type
        self.set_mode("triptych")

        try: import SimpleITK as sitk
        except: return

        if sitk_image is not None:
            img = sitk_image
        else:
            try:
                img = sitk.GetImageFromArray(vol.astype(np.float32))
                img.SetSpacing((float(spacing[2]), float(spacing[1]), float(spacing[0])))
                if origin is not None: img.SetOrigin(tuple(float(x) for x in origin))
                if direction is not None:
                    d = np.array(direction, dtype=float).reshape(3, 3)
                    img.SetDirection(d.ravel(order="C").tolist())
            except: return

        self._sitk_base_img = img
        self._overlay_cache.clear()

        # We keep _vol_SAR and _spacing_SAR as our clinical reference (Superior-Anterior-Right)
        # but we don't apply global transforms anymore.
        vol_SAR, spacing_SAR = self._reorient_image_to_SAR_numpy(img)
        self._vol_SAR = vol_SAR
        self._spacing_SAR = spacing_SAR
        
        # In decentralized mode, we just use SAR directly as our logical volume
        self._vol_log = vol_SAR
        self._spacing_log = spacing_SAR
        self._perm_phys_to_log = (0, 1, 2) # S-A-R
        
        # Reset and Refresh
        lo, hi = _robust_levels(self._vol_log, 0.5, 99.5)
        self._levels = (lo, hi)

        # Initial Smart Mapping: Try to match indices to anatomical names based on SAR metadata
        S, A, R = self._vol_log.shape
        self._cS, self._cA, self._cR = S // 2, A // 2, R // 2
        self._iS, self._iA, self._iR = self._cS, self._cA, self._cR

        # The mapping we use: Logical Volume index -> Anatomical Name
        # Based on _build_logical_from_physical:
        # through=='S' (S-A-R) -> (0:S, 1:A, 2:R) -> (0:Axial, 1:Coronal, 2:Sagittal)
        # through=='A' (A-S-R) -> (0:A, 1:S, 2:R) -> (0:Coronal, 1:Axial, 2:Sagittal)
        # through=='R' (R-A-S) -> (0:R, 1:A, 2:S) -> (0:Sagittal, 1:Coronal, 2:Axial)
        
        # We find which physical index is S, A, R
        indices = {0: "Axial", 1: "Coronal", 2: "Sagittal"}
        # But we set it based on the pane default for now, then override via signals if user changes.
        self._axis_to_name = {0: "Axial", 1: "Coronal", 2: "Sagittal"}
        
        # Refresh UI
        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            p.cmb_view.blockSignals(True)
            p.cmb_view.setCurrentText(self._axis_to_name[p.axis_index])
            p.cmb_view.blockSignals(False)

        # Update sliders in all panes
        self.pane_ax.sld.setEnabled(True); self.pane_ax.sld.setRange(-self._cS, (S-1)-self._cS)
        self.pane_co.sld.setEnabled(True); self.pane_co.sld.setRange(-self._cA, (A-1)-self._cA)
        self.pane_sa.sld.setEnabled(True); self.pane_sa.sld.setRange(-self._cR, (R-1)-self._cR)

        self._sync_sliders_from_state()
        self._update_triptych()

    def get_effective_orientation(self):
        """
        Calculates transpose axes based on current manual label assignments.
        """
        name_to_idx = {v: k for k, v in self._axis_to_name.items()}
        ax_idx = name_to_idx.get("Axial", 0)
        co_idx = name_to_idx.get("Coronal", 1)
        sa_idx = name_to_idx.get("Sagittal", 2)
        # This mapping represents which logical index becomes Axial, Coronal, Sagittal
        # Pipeline expects (idx_for_ax, idx_for_co, idx_for_sa)
        t_axes = (ax_idx, co_idx, sa_idx)
        # Rotation is still pulled from the pane labeled 'Axial'
        rk = 0
        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            if self._axis_to_name[p.axis_index] == "Axial":
                rk = p.rotation_k
                break
        return t_axes, rk

    def _reorient_image_to_SAR_numpy(self, img):
        """
        Reorient sitk image array to SAR (Superior-Anterior-Right) orientation.
        Also stores reorientation parameters for applying to overlays.
        """
        import SimpleITK as sitk
        arr_zyx = sitk.GetArrayFromImage(img).astype(np.float32)
        D_lps = np.array(img.GetDirection(), dtype=float).reshape(3,3)
        L2R = np.diag([-1.0, -1.0, 1.0])
        D_ras = L2R @ D_lps
        axes = ['I','J','K']; anat = ['R','A','S']
        idx_for = {}; sign_for = {}; used=set()
        for t_idx, t_lbl in enumerate(anat):
            best_axis=None; best_val=-1.0; best_sign=+1
            for col, name in enumerate(axes):
                if name in used: continue
                v = D_ras[:, col]; val = abs(v[t_idx])
                if val > best_val:
                    best_val = val; best_axis = name; best_sign = +1 if v[t_idx] >= 0 else -1
            idx_for[t_lbl] = best_axis; sign_for[t_lbl] = best_sign; used.add(best_axis)
        name_to_npaxis_zyx = {'K':0,'J':1,'I':2}
        perm = (name_to_npaxis_zyx[idx_for['S']], name_to_npaxis_zyx[idx_for['A']], name_to_npaxis_zyx[idx_for['R']])
        vol = np.transpose(arr_zyx, perm)
        if sign_for['S'] < 0: vol = vol[::-1,:,:]
        if sign_for['A'] < 0: vol = vol[:,::-1,:]
        if sign_for['R'] < 0: vol = vol[:,:,::-1]
        sx_I, sy_J, sz_K = img.GetSpacing()
        spacing_map = {'I': float(sx_I), 'J': float(sy_J), 'K': float(sz_K)}
        spacing_SAR = (spacing_map[idx_for['S']], spacing_map[idx_for['A']], spacing_map[idx_for['R']])
        
        # Store reorientation params for overlay processing
        self._sar_reorient_params = {
            'perm_zyx_to_SAR': perm,
            'flips_SAR': (sign_for['S'] < 0, sign_for['A'] < 0, sign_for['R'] < 0),
        }
        
        return vol, spacing_SAR

    def _build_logical_from_physical(self, vol_SAR, spacing_SAR):
        S, A, R = map(int, vol_SAR.shape)
        s_mm, a_mm, r_mm = map(float, spacing_SAR)
        spacing_rank = sorted([('S',s_mm),('A',a_mm),('R',r_mm)], key=lambda x: x[1], reverse=True)
        size_rank    = sorted([('S',S),('A',A),('R',R)],           key=lambda x: x[1])
        score = {'S':0,'A':0,'R':0}
        for i,(ax,_) in enumerate(spacing_rank): score[ax] += (2-i)
        for i,(ax,_) in enumerate(size_rank):    score[ax] += (2-i)
        through = max(['S','A','R'], key=lambda ax: score[ax])
        if through == 'S':
            perm = (0,1,2); spacing_log = (s_mm, a_mm, r_mm)
        elif through == 'A':
            perm = (1,0,2); spacing_log = (a_mm, s_mm, r_mm)
        else:
            perm = (2,1,0); spacing_log = (r_mm, a_mm, s_mm)
        vol_log = np.transpose(vol_SAR, perm)
        return vol_log, spacing_log

    def is_volume_loaded(self) -> bool:
        """Returns True if a 3D volume is currently loaded and ready for triptych display."""
        return self._vol_log is not None

    def _sync_sliders_from_state(self):
        for p, i, c in ((self.pane_ax, self._iS, self._cS), (self.pane_co, self._iA, self._cA), (self.pane_sa, self._iR, self._cR)):
            p.sld.blockSignals(True)
            p.sld.setValue(i - c)
            p.sld.blockSignals(False)

    def _on_slice_changed(self, _val: int):
        if self._vol_log is None: return
        sender = self.sender()
        if isinstance(sender, CTPane):
            vt = sender.view_type
            val = sender.sld.value()
            if vt == "Axial": self._iS = self._cS + val
            elif vt == "Coronal": self._iA = self._cA + val
            else: self._iR = self._cR + val
            
            # Sync others
            self._sync_sliders_from_state()
            self._update_triptych()
            self._update_overlay_texts()
            self.slices_changed.emit(self._iS, self._iA, self._iR)

    def _on_pane_settings_changed(self):
        if self._vol_log is None: return
        sender = self.sender()
        if isinstance(sender, CTPane):
            idx = sender.axis_index
            new_name = sender.view_type
            old_name = self._axis_to_name[idx]
            if new_name != old_name:
                # Swap logic
                other_idx = next(i for i, n in self._axis_to_name.items() if n == new_name)
                self._axis_to_name[idx] = new_name
                self._axis_to_name[other_idx] = old_name
                
                # Update all combo boxes
                for p in (self.pane_ax, self.pane_co, self.pane_sa):
                    p.cmb_view.blockSignals(True)
                    p.cmb_view.setCurrentText(self._axis_to_name[p.axis_index])
                    p.cmb_view.blockSignals(False)

        # Update slider ranges based on new anatomical meanings
        S, A, R = self._vol_log.shape
        # We need to knows which INDEX is Axial, etc.
        name_to_idx = {v: k for k, v in self._axis_to_name.items()}
        cents = {"Axial": self._cS, "Coronal": self._cA, "Sagittal": self._cR}
        dims  = {"Axial": S, "Coronal": A, "Sagittal": R}

        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            # p.axis_index is fixed. What is its name now?
            nm = self._axis_to_name[p.axis_index]
            d, c = dims[nm], cents[nm]
            p.sld.blockSignals(True)
            p.sld.setRange(-c, (d-1)-c)
            p.sld.blockSignals(False)

        self._sync_sliders_from_state()
        self._update_triptych()

    def _update_overlay_texts(self):
        if not self._spacing_log or self._vol_log is None: return
        s_mm, a_mm, r_mm = self._spacing_log
        mm_map = {"Axial": s_mm, "Coronal": a_mm, "Sagittal": r_mm}
        idx_map = {"Axial": (self._iS, self._cS), "Coronal": (self._iA, self._cA), "Sagittal": (self._iR, self._cR)}
        
        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            name = self._axis_to_name[p.axis_index]
            cur_i, cent_i = idx_map[name]
            spacing = mm_map[name]
            p.lbl.set_br_text(f"{(cur_i - cent_i) * spacing:.1f} mm")

    def _update_triptych(self):
        if self._vol_log is None or self._levels is None: return
        lo, hi = self._levels
        overlay_color = OVERLAY_COLORS.get(self._overlay_name, (255, 200, 0))

        def _get_slice_tuple(pane):
            idx = pane.axis_index
            name = self._axis_to_name[idx]
            # Use the index that matches the assigned anatomical name
            slice_num = self._iS if name == "Axial" else (self._iA if name == "Coronal" else self._iR)
            
            if idx == 0: return self._vol_log[slice_num, :, :], (self._ov_log[slice_num, :, :] if self._ov_log is not None else None)
            if idx == 1: return self._vol_log[:, slice_num, :], (self._ov_log[:, slice_num, :] if self._ov_log is not None else None)
            return self._vol_log[:, :, slice_num], (self._ov_log[:, :, slice_num] if self._ov_log is not None else None)

        def _blend(u8, mask, color, alpha=0.5):
            rgb = np.repeat(u8[..., None], 3, axis=2).astype(np.float32)
            if mask is not None and mask.shape == u8.shape:
                m = mask.astype(bool)
                for ch, val in enumerate(color):
                    rgb[..., ch][m] = (1.0 - alpha) * rgb[..., ch][m] + alpha * val
            return np.clip(rgb, 0, 255).astype(np.uint8)

        for pane in (self.pane_ax, self.pane_co, self.pane_sa):
            slc, msk = _get_slice_tuple(pane)
            # Normalization
            img = np.flipud(np.fliplr(slc))
            if msk is not None: msk = np.flipud(np.fliplr(msk))
            
            # Local transforms
            if pane.rotation_k != 0:
                img = np.rot90(img, k=pane.rotation_k)
                if msk is not None: msk = np.rot90(msk, k=pane.rotation_k)
            if pane.flip_h:
                img = np.fliplr(img)
                if msk is not None: msk = np.fliplr(msk)
            if pane.flip_v:
                img = np.flipud(img)
                if msk is not None: msk = np.flipud(msk)

            u8 = _window_to_u8_levels(img, lo, hi)
            rgb = _blend(u8, msk, overlay_color)
            pane.lbl.setPixmap(_to_qpixmap_rgb(rgb))

        self._rescale_all()

    def _get_scale_for_view_by_index(self, axis_idx: int):
        if not self._spacing_log: return (1.0, 1.0)
        s, a, r = self._spacing_log
        # Slicing axis 0 (S) -> show A and R dimensions. Width=R, Height=A
        if axis_idx == 0:   fx, fy = r, a
        # Slicing axis 1 (A) -> show S and R dimensions. Width=R, Height=S
        elif axis_idx == 1: fx, fy = r, s
        # Slicing axis 2 (R) -> show S and A dimensions. Width=A, Height=S
        else:               fx, fy = a, s
        m = max(fx, fy, 1e-6)
        return fx/m, fy/m

    def _rescale_all(self):
        if self._mode == "single":
            self._set_scaled_fit(self.pane_ax.lbl, self.pane_ax.lbl.pixmap(), 1.0, 1.0)
            return
        for p in (self.pane_ax, self.pane_co, self.pane_sa):
            sc = self._get_scale_for_view_by_index(p.axis_index)
            self._set_scaled_fit(p.lbl, p.lbl.pixmap(), *sc)

    def _set_scaled_fit(self, lbl: QLabel, pm: QPixmap | None, fx: float = 1.0, fy: float = 1.0):
        if not pm or pm.isNull(): return
        w, h = lbl.width(), lbl.height()
        if w <= 0 or h <= 0: return
        pw, ph = pm.width(), pm.height()
        if pw <= 0 or ph <= 0: return
        s = min(w / (pw * fx), h / (ph * fy))
        tw = max(1, int(pw * fx * s)); th = max(1, int(ph * fy * s))
        lbl.setPixmap(pm.scaled(tw, th, Qt.AspectRatioMode.IgnoreAspectRatio,
                                Qt.TransformationMode.SmoothTransformation))

    # Legacy DICOM previews
    def set_axial_pixmap(self, pix_ax: QPixmap | None):
        if self._mode != "single" or self._vol_log is not None or not pix_ax:
            return
        self._pm_ax = pix_ax
        self._set_scaled_fit(self.lbl_ax, pix_ax, 1.0, 1.0)
        QTimer.singleShot(0, lambda: self._set_scaled_fit(self.lbl_ax, self._pm_ax, 1.0, 1.0))

    def set_pixmaps(self, pix_ax, pix_co, pix_sa):
        if self._vol_log is not None:
            return
        self._pm_ax, self._pm_co, self._pm_sa = pix_ax, pix_co, pix_sa
        self._rescale_all()

    def showEvent(self, e):
        super().showEvent(e)
        QTimer.singleShot(50, self._rescale_all)

    def resizeEvent(self, e):
        self._rescale_all()
        super().resizeEvent(e)

# ===============================================
# DRR Cine Viewer
# ===============================================
from ..ui.info_panels import DRRInfoWidget


class DRRViewerPanel(QGroupBox):
    def __init__(self, title: str = "Digitally Reconstructed Radiographs (DRRs)"):
        super().__init__(title)
        
        # State
        self._ct_data = {}
        self._current_ct = None
        self._current_frame = 0
        self._is_playing = False
        self._playback_fps = 15
        self._projections_per_ct = 120 # Default

        # Build UI
        root_layout = QVBoxLayout(self)
        root_layout.setSpacing(8)
        
        # 1. Splitter or Side-by-Side layout (Image Left, Info Right)
        # To match CTQuadPanel look, we can put Info on right
        content_layout = QHBoxLayout()
        
        # Left: Viewer + Controls
        viewer_container = QWidget()
        v_lay = QVBoxLayout(viewer_container)
        v_lay.setContentsMargins(0,0,0,0)

        self.main_view = QLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self.main_view.setMinimumSize(600, 500)
        self.main_view.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.main_view.setStyleSheet("background-color: #000; border: 1px solid #444;")
        v_lay.addWidget(self.main_view, 1)

        # Controls
        controls = QHBoxLayout()
        self.btn_play = QPushButton("▶")
        self.btn_play.setFixedWidth(50); self.btn_play.setCheckable(True)
        self.btn_play.clicked.connect(self._on_play_pause)
        controls.addWidget(self.btn_play)
        
        self.slider = QSlider(Qt.Orientation.Horizontal)
        self.slider.setRange(0, self._projections_per_ct - 1)
        self.slider.valueChanged.connect(self._on_slider_changed)
        controls.addWidget(self.slider, 1)
        
        self.spin_speed = QComboBox()
        self.spin_speed.addItems(["5 fps", "10 fps", "15 fps", "30 fps", "60 fps"])
        self.spin_speed.setCurrentText("15 fps")
        self.spin_speed.currentTextChanged.connect(self._on_speed_changed)
        controls.addWidget(QLabel("Speed:"))
        controls.addWidget(self.spin_speed)
        v_lay.addLayout(controls)

        self._grp_view_display = QGroupBox("Display (on-screen only; does not change saved PNGs)")
        disp = self._grp_view_display
        disp.setStyleSheet("QGroupBox { font-size: 10pt; } QGroupBox::title { subcontrol-origin: margin; left: 8px; }")
        disp_l = QVBoxLayout(disp)
        disp_l.setSpacing(4)
        r1 = QHBoxLayout()
        r1.addWidget(QLabel("Rotate:"))
        self._spin_view_rotate = QDoubleSpinBox()
        self._spin_view_rotate.setRange(-360.0, 360.0)
        self._spin_view_rotate.setDecimals(2)
        self._spin_view_rotate.setSingleStep(0.5)
        self._spin_view_rotate.setSuffix("°")
        self._spin_view_rotate.setMinimumWidth(88)
        self._spin_view_rotate.valueChanged.connect(self._on_view_transform_changed)
        r1.addWidget(self._spin_view_rotate)
        for deg, lab in ((0, "0°"), (90, "90°"), (180, "180°"), (270, "270°")):
            b = QPushButton(lab)
            b.setFixedWidth(36)
            b.clicked.connect(lambda _, d=deg: self._spin_view_rotate.setValue(float(d)))
            r1.addWidget(b)
        bm = QPushButton("−90°")
        bm.setFixedWidth(44)
        bm.clicked.connect(lambda: self._spin_view_rotate.setValue(self._spin_view_rotate.value() - 90.0))
        bp = QPushButton("+90°")
        bp.setFixedWidth(44)
        bp.clicked.connect(lambda: self._spin_view_rotate.setValue(self._spin_view_rotate.value() + 90.0))
        r1.addWidget(bm)
        r1.addWidget(bp)
        br = QPushButton("Reset")
        br.setToolTip("Rotation 0°, no flips")
        br.clicked.connect(self._reset_view_display)
        r1.addWidget(br)
        r1.addStretch(1)
        disp_l.addLayout(r1)
        r2 = QHBoxLayout()
        self._chk_view_flip_h = QCheckBox("Flip horizontal")
        self._chk_view_flip_v = QCheckBox("Flip vertical")
        self._chk_view_transpose = QCheckBox("Transpose (swap W↔H)")
        self._chk_view_transpose.setToolTip("Mirror across the main diagonal (like matrix transpose)")
        for c in (self._chk_view_flip_h, self._chk_view_flip_v, self._chk_view_transpose):
            c.toggled.connect(self._on_view_transform_changed)
        r2.addWidget(self._chk_view_flip_h)
        r2.addWidget(self._chk_view_flip_v)
        r2.addWidget(self._chk_view_transpose)
        r2.addStretch(1)
        disp_l.addLayout(r2)
        v_lay.addWidget(disp)
        
        # Right: Info Panel
        self.info_panel = DRRInfoWidget()
        self.info_panel.setFixedWidth(280)
        self.info_panel.ct_changed.connect(self._on_ct_changed)

        content_layout.addWidget(viewer_container, 1)
        content_layout.addWidget(self.info_panel, 0)
        
        root_layout.addLayout(content_layout)

        # Loading state
        self.lbl_status = QLabel("Waiting for DRR generation...")
        self.lbl_status.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.lbl_status.setStyleSheet("color: #999; font-size: 11pt;")

        self.playback_timer = QTimer(self)
        self.playback_timer.timeout.connect(self._on_timer_tick)
        self._set_controls_enabled(False)

    # ... [Keep clear(), set_loading_state(), add_frame(), add_ct_frames(), etc.] ...
    # BUT update references to use self.info_panel.set_status() instead of setting labels directly

    def clear(self):
        self._stop_playback()
        self._ct_data.clear()
        self._current_ct = None
        self._current_frame = 0
        self.info_panel.update_ct_list([], [])
        self.info_panel.set_status("—", "—", "—")
        self.main_view.clear()
        self.main_view.setText("Waiting for DRR generation...")
        self._reset_view_display(silent=True)
        self._set_controls_enabled(False)

    def copy_view_display_settings_from(self, src: "DRRViewerPanel") -> None:
        """Match on-screen transform controls (e.g. when cloning a step tab)."""
        self._spin_view_rotate.blockSignals(True)
        self._spin_view_rotate.setValue(src._spin_view_rotate.value())
        self._spin_view_rotate.blockSignals(False)
        self._chk_view_flip_h.setChecked(src._chk_view_flip_h.isChecked())
        self._chk_view_flip_v.setChecked(src._chk_view_flip_v.isChecked())
        self._chk_view_transpose.setChecked(src._chk_view_transpose.isChecked())

    def _reset_view_display(self, silent: bool = False) -> None:
        self._spin_view_rotate.blockSignals(True)
        self._spin_view_rotate.setValue(0.0)
        self._spin_view_rotate.blockSignals(False)
        self._chk_view_flip_h.setChecked(False)
        self._chk_view_flip_v.setChecked(False)
        self._chk_view_transpose.setChecked(False)
        if not silent and self._current_ct is not None:
            self._display_frame(self._current_frame)

    def _on_view_transform_changed(self, *_args) -> None:
        if self._current_ct is not None:
            self._display_frame(self._current_frame)

    def _maybe_transpose_pixmap(self, pm: QPixmap) -> QPixmap:
        """Swap image axes (x↔y), same as matrix transpose on pixel indices."""
        if not self._chk_view_transpose.isChecked() or pm.isNull():
            return pm
        # x' = y, y' = x  →  m11=0,m12=1,m21=1,m22=0 in Qt's column-major 2×2 part
        tr = QTransform(0, 1, 1, 0, 0, 0)
        return pm.transformed(tr, Qt.TransformationMode.SmoothTransformation)

    def _transform_pixmap_for_view(self, src: QPixmap) -> QPixmap:
        """Apply transpose (optional), flips, then rotation around image centre."""
        if src.isNull():
            return src
        p = self._maybe_transpose_pixmap(src)
        w, h = p.width(), p.height()
        if w <= 0 or h <= 0:
            return p
        t = QTransform()
        t.translate(w / 2.0, h / 2.0)
        if self._chk_view_flip_h.isChecked():
            t.scale(-1.0, 1.0)
        if self._chk_view_flip_v.isChecked():
            t.scale(1.0, -1.0)
        t.rotate(self._spin_view_rotate.value())
        t.translate(-w / 2.0, -h / 2.0)
        return p.transformed(t, Qt.TransformationMode.SmoothTransformation)
    
    def set_total_projections(self, count: int):
        """Update the expected total number of projections per CT."""
        if count > 0:
            self._projections_per_ct = count
            self.slider.setRange(0, count - 1)
            # Update current view if frame is visible
            if self._current_ct is not None:
                self._display_frame(self._current_frame)

    def set_loading_state(self, ct_count: int):
        self.clear()
        if ct_count > 0:
            msg = (
                f"Generating DRRs from {ct_count} CT volume(s) (Joseph forward projection).\n\n"
                "PNG projections appear here as they are written (bone = white, air = black)."
            )
        else:
            msg = "No CT volumes found."
        self.main_view.setText(msg)
        self.main_view.setStyleSheet("color: #9aa0a6; font-size: 12pt;")
        self.main_view.setAlignment(Qt.AlignmentFlag.AlignCenter)

    def add_frame(self, ct_index: int, proj_index: int, ct_name: str, frame: object, angle: float):
        if ct_index not in self._ct_data:
            self._ct_data[ct_index] = {'name': ct_name, 'frames': {}, 'angles': {}, 'files': {}}
            self._refresh_ct_combo()

        ct_data = self._ct_data[ct_index]
        ct_data['frames'][proj_index] = frame
        ct_data['angles'][proj_index] = angle

        if self._current_ct is None:
            self._current_ct = ct_index
            self._set_controls_enabled(True)
            self.main_view.setText("")

        # Avoid repainting the main view for every projection during bulk load (COMPRESS → many .bin
        # updates): only refresh when this slot is the one the user is viewing.
        if self._current_ct == ct_index and proj_index == self._current_frame:
            self._display_frame(proj_index)

    def add_ct_frames(self, ct_index, ct_name, files, frames, angles):
        self._ct_data[ct_index] = {'name': ct_name, 'files': files, 'frames': frames, 'angles': angles}
        self._refresh_ct_combo()
        self._set_controls_enabled(True)

    def _refresh_ct_combo(self):
        names = []
        ids = []
        for idx, data in self._ct_data.items():
            names.append(data['name'])
            ids.append(idx)
        self.info_panel.update_ct_list(names, ids)

    def _display_ct(self, ct_index: int):
        if ct_index not in self._ct_data: return
        self._stop_playback()
        self._current_ct = ct_index
        data = self._ct_data[ct_index]
        
        # Try to show first frame
        keys = sorted(data['frames'].keys())
        if keys:
            self._display_frame(keys[0])
        else:
            self.info_panel.set_status("—", "—", "—")

    def _display_frame(self, frame_idx: int):
        if self._current_ct is None:
            return
        data = self._ct_data[self._current_ct]

        if frame_idx not in data["frames"]:
            return

        self._current_frame = frame_idx
        pixmap = data["frames"][frame_idx]
        angle = data["angles"][frame_idx]

        if pixmap and not pixmap.isNull():
            self.main_view.setStyleSheet("background-color: #000; border: 1px solid #444;")
            transformed = self._transform_pixmap_for_view(pixmap)
            scaled = transformed.scaled(
                self.main_view.size(),
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation,
            )
            self.main_view.setPixmap(scaled)

        w, h = pixmap.width(), pixmap.height()
        gantry = f"{angle:.1f}°"
        vr = self._spin_view_rotate.value()
        extras = []
        if abs(vr) > 1e-6:
            extras.append(f"view {vr:+.1f}°")
        if self._chk_view_flip_h.isChecked():
            extras.append("flipH")
        if self._chk_view_flip_v.isChecked():
            extras.append("flipV")
        if self._chk_view_transpose.isChecked():
            extras.append("T")
        if extras:
            gantry = f"{gantry} ({', '.join(extras)})"
        self.info_panel.set_status(
            f"{frame_idx + 1} / {self._projections_per_ct}", gantry, f"{w}×{h}"
        )

        self.slider.blockSignals(True)
        self.slider.setValue(frame_idx)
        self.slider.blockSignals(False)

    # ... [Keep playback controls: _on_play_pause, _start_playback, _stop_playback, _on_timer_tick, _on_slider_changed, _on_speed_changed] ...
    def _on_play_pause(self):
        if self.btn_play.isChecked(): self._start_playback()
        else: self._stop_playback()
    
    def _start_playback(self):
        if self._current_ct is None: return
        self._is_playing = True; self.btn_play.setText("⏸"); self.playback_timer.start(int(1000/self._playback_fps))
    
    def _stop_playback(self):
        self._is_playing = False; self.playback_timer.stop(); self.btn_play.setChecked(False); self.btn_play.setText("▶")
    
    def _on_timer_tick(self):
        if self._current_ct is None: return self._stop_playback()
        data = self._ct_data[self._current_ct]
        sorted_ids = sorted(data['frames'].keys())
        if not sorted_ids: return
        try:
            curr_idx = sorted_ids.index(self._current_frame)
            next_idx = (curr_idx + 1) % len(sorted_ids)
            self._display_frame(sorted_ids[next_idx])
        except ValueError:
            self._display_frame(sorted_ids[0])

    def _on_slider_changed(self, val):
        if self._is_playing: self._stop_playback()
        self._display_frame(val)
        
    def _on_speed_changed(self, txt):
        try: self._playback_fps = int(txt.split()[0])
        except: pass
        
    def _on_ct_changed(self, idx):
        ct_id = self.info_panel.cmb_ct.itemData(idx)
        if ct_id is not None: self._display_ct(ct_id)

    def _set_controls_enabled(self, en):
        self.btn_play.setEnabled(en)
        self.slider.setEnabled(en)
        self.spin_speed.setEnabled(en)
        # Display transforms stay enabled so rotation/flips can be preset before frames exist.
        self._grp_view_display.setEnabled(True)

    def resizeEvent(self, e):
        super().resizeEvent(e)
        if self._current_ct is not None:
            self._display_frame(self._current_frame)


# ===============================================
# DVF HELPER FUNCTIONS
# ===============================================

def bgr_colormap(norm: np.ndarray) -> np.ndarray:
    """Vectorized piecewise-linear map: 0=blue, 0.3=green, 1=red."""
    n = np.clip(norm.astype(np.float32), 0.0, 1.0)
    rgb = np.zeros(n.shape + (3,), dtype=np.float32)

    pivot = 0.3 # Green at 30%

    m1 = n <= pivot
    m2 = ~m1
    if np.any(m1):
        # Blue -> Green ramp (0.0 to pivot)
        t = (n[m1] / pivot)
        rgb[m1, 0] = 0.0              # R
        rgb[m1, 1] = 255.0 * t        # G: 0→255
        rgb[m1, 2] = 255.0 * (1.0 - t)  # B: 255→0
    if np.any(m2):
        # Green -> Red ramp (pivot to 1.0)
        t = (n[m2] - pivot) / (1.0 - pivot)
        rgb[m2, 0] = 255.0 * t        # R: 0→255
        rgb[m2, 1] = 255.0 * (1.0 - t)  # G: 255→0
        rgb[m2, 2] = 0.0              # B
    return rgb

def nice_ceil_mm(x: float) -> float:
    """Round up to a 'nice' upper bound in millimetres."""
    import math
    if x <= 0:
        return 1.0
    return float(math.ceil(x))


def _dvf_scalar_zyx_to_log_volume(
    arr_zyx: np.ndarray,
    sar_params: dict,
    perm_phys_to_log: tuple[int, int, int] | None,
) -> np.ndarray:
    """Same spatial mapping as scalar DVF magnitude (zyx array from GetArrayFromImage)."""
    perm = sar_params["perm_zyx_to_SAR"]
    flips = sar_params["flips_SAR"]
    vol_SAR = np.transpose(arr_zyx, perm)
    if flips[0]:
        vol_SAR = vol_SAR[::-1, :, :]
    if flips[1]:
        vol_SAR = vol_SAR[:, ::-1, :]
    if flips[2]:
        vol_SAR = vol_SAR[:, :, ::-1]
    if perm_phys_to_log is None:
        return vol_SAR
    return np.transpose(vol_SAR, perm_phys_to_log)


def _dvf_inplane_mm(
    vec_log: np.ndarray,
    perm_phys_to_log: tuple[int, int, int] | None,
    iS: int,
    iA: int,
    iR: int,
    view: str,
) -> tuple[np.ndarray, np.ndarray]:
    """
    In-plane displacement (mm) for axial / coronal / sagittal slice.
    vec_log[..., c] follows the same component order as ITK after per-channel resample.
    perm_phys_to_log maps logical slice axes to source component indices (numpy transpose tuple).
    """
    p = perm_phys_to_log if perm_phys_to_log is not None else (0, 1, 2)
    if view == "ax":
        va = vec_log[iS, :, :, p[1]]
        vb = vec_log[iS, :, :, p[2]]
    elif view == "co":
        va = vec_log[:, iA, :, p[0]]
        vb = vec_log[:, iA, :, p[2]]
    else:
        va = vec_log[:, :, iR, p[0]]
        vb = vec_log[:, :, iR, p[1]]
    return va.astype(np.float32), vb.astype(np.float32)


def _overlay_arrows_on_rgb(
    rgb: np.ndarray,
    u: np.ndarray,
    v: np.ndarray,
    row_spacing_mm: float,
    col_spacing_mm: float,
    *,
    step: int,
    length_scale: float,
    max_arrows: int,
    min_mag_mm: float,
) -> None:
    """Draw arrows on float RGB (H,W,3) in place. u = along row (increasing y), v = along col (increasing x)."""
    if rgb.ndim != 3 or rgb.shape[2] != 3:
        return
    H, W = u.shape
    if v.shape != (H, W):
        return
    inv_sr = 1.0 / max(float(row_spacing_mm), 1e-6)
    inv_sc = 1.0 / max(float(col_spacing_mm), 1e-6)
    mag = np.sqrt(u * u + v * v)
    pts: list[tuple[int, int]] = []
    st = max(1, int(step))
    for i in range(0, H, st):
        for j in range(0, W, st):
            if mag[i, j] >= min_mag_mm:
                pts.append((i, j))
    if not pts:
        return
    if len(pts) > max_arrows:
        stride = max(1, int(np.ceil(len(pts) / float(max_arrows))))
        pts = pts[::stride][:max_arrows]

    col = np.array([255.0, 235.0, 80.0], dtype=np.float32)

    def _pix(yy: int, xx: int, a: float = 1.0) -> None:
        if 0 <= yy < H and 0 <= xx < W:
            rgb[yy, xx] = (1.0 - a) * rgb[yy, xx] + a * col

    for i, j in pts:
        du = float(u[i, j]) * inv_sr * length_scale
        dv = float(v[i, j]) * inv_sc * length_scale
        y1 = i + du
        x1 = j + dv
        length = float(np.hypot(x1 - j, y1 - i))
        if length < 0.5:
            continue
        x0, y0 = float(j), float(i)
        n = max(int(np.ceil(length)), 1)
        for t in range(n + 1):
            tt = t / n
            y = int(round(y0 + tt * (y1 - y0)))
            x = int(round(x0 + tt * (x1 - x0)))
            _pix(y, x, 0.85)
        ang = np.arctan2(y1 - y0, x1 - x0)
        ah = max(2.5, min(8.0, 0.22 * length))
        for sign in (-1.0, 1.0):
            a2 = ang + sign * np.deg2rad(150.0)
            hx = x1 + ah * np.cos(a2)
            hy = y1 + ah * np.sin(a2)
            hn = max(int(np.ceil(ah)), 1)
            for t in range(hn + 1):
                tt = t / hn
                y = int(round(y1 + tt * (hy - y1)))
                x = int(round(x1 + tt * (hx - x1)))
                _pix(y, x, 0.75)


# ===============================================
# DVF PANEL CLASS
# ===============================================

class DVFPanel(CTQuadPanel):
    """
    Integrated DVF Viewer.
    Inherits from CTQuadPanel for the 3-pane layout but uses 
    DVFInfoWidget for specific DVF controls.
    """
    def __init__(self, *a, **kw):
        # 1. Instantiate the specific info widget
        self.dvf_info = DVFInfoWidget()
        
        # 2. Pass it up to CTQuadPanel so it gets inserted into the layout
        # title is the first positional argument of CTQuadPanel.__init__
        title = a[0] if len(a) > 0 else "DVF Viewer"
        super().__init__(title=title, custom_info_widget=self.dvf_info)

        # 3. Connect DVF-specific signals from the info widget
        self.dvf_info.dvf_changed.connect(self._on_dvf_phase_changed)
        self.dvf_info.opacity_changed.connect(self._on_alpha_changed)
        self.dvf_info.rebind_changed.connect(lambda checked: self.apply_plane_swap(checked))
        self.dvf_info.arrow_settings_changed.connect(self._update_triptych)

        # 4. DVF Data State
        self._dvf_paths: Dict[str, Path] = {}
        self._dvf_sources: dict[str, str] = {}
        # path resolve key -> (mtime_ns, size, max magnitude) to avoid re-reading all DVFs on each poll
        self._dvf_vmax_file_cache: Dict[str, Tuple[int, int, float]] = {}
        
        self._dvf_log0: Optional[np.ndarray] = None   # magnitude in original logical (A,S,R)
        self._dvf_log: Optional[np.ndarray] = None    # magnitude in current logical (may be S,A,R)
        self._dvf_vec_log0: Optional[np.ndarray] = None  # (S,A,R,3) mm, same grid as _dvf_log0
        self._dvf_vec_log: Optional[np.ndarray] = None
        self._dvf_vmax: float = 10.0
        self._dvf_alpha: float = 0.45 # Default opacity matches slider default
        
        # 5. Install Event Filters for Hover Inspection
        # (Labels are created by parent CTQuadPanel)
        for lbl in (self.lbl_ax, self.lbl_co, self.lbl_sa):
            lbl.setMouseTracking(True)
            lbl.installEventFilter(self)

        # 6. Logic State (for plane swapping)
        self._swap_active = False
        self._vol_log0 = None
        self._spacing_log0 = None
        self._levels0 = None

        # Initialize scale bar
        self._update_dvf_legend()

    # ------------- plane swap (rebinding) -------------
    def set_mha_volume(self, vol, spacing, origin=None, direction=None, sitk_image=None, dataset_type="clinical"):
        # Call parent to set up base logical state and reset indices
        super().set_mha_volume(vol, spacing, origin, direction, sitk_image=sitk_image, dataset_type=dataset_type)
        
        # Cache the "original" logical state from the parent
        self._vol_log0 = getattr(self, "_vol_log", None)
        self._spacing_log0 = getattr(self, "_spacing_log", None)
        self._levels0 = getattr(self, "_levels", None)
        
        # When a new CT is loaded, we must also reset the DVF cache
        self._dvf_log0 = None
        self._dvf_log = None
        self._dvf_vec_log0 = None
        self._dvf_vec_log = None

        # Explicitly reset slice indices to center
        if self._cS is not None:
            self._iS, self._iA, self._iR = self._cS, self._cA, self._cR
        
        # Apply swap state if active
        if self._swap_active:
            self.apply_plane_swap(True)
        else:
            if self._vol_log is not None and self._spacing_log is not None:
                self._assign_logical(self._vol_log, self._spacing_log)

        self._update_triptych()

    def apply_plane_swap(self, active: bool):
        self._swap_active = bool(active)
        
        # Case 1: Only DVF is loaded (no CT volume)
        if self._vol_log0 is None or self._spacing_log0 is None:
            if self._dvf_log0 is not None:
                # Toggle logic for DVF-only
                self._dvf_log = np.transpose(self._dvf_log0, (1, 0, 2)) if active else self._dvf_log0
                if self._dvf_vec_log0 is not None:
                    self._dvf_vec_log = (
                        np.transpose(self._dvf_vec_log0, (1, 0, 2, 3)) if active else self._dvf_vec_log0
                    )
                self._update_triptych()
            return
            
        # Case 2: CT Volume + DVF
        if not self._swap_active:
            # TOGGLE OFF (default): Original logical state (A,S,R)
            vol_to_assign = self._vol_log0
            spacing_to_assign = self._spacing_log0
            self._dvf_log = self._dvf_log0 if self._dvf_log0 is not None else None
            self._dvf_vec_log = self._dvf_vec_log0 if self._dvf_vec_log0 is not None else None
        else:
            # TOGGLE ON (swapped): Swap S <-> A
            vol_to_assign = np.transpose(self._vol_log0, (1, 0, 2))
            s0, a0, r0 = self._spacing_log0
            spacing_to_assign = (a0, s0, r0)
            self._dvf_log = np.transpose(self._dvf_log0, (1, 0, 2)) if self._dvf_log0 is not None else None
            self._dvf_vec_log = (
                np.transpose(self._dvf_vec_log0, (1, 0, 2, 3)) if self._dvf_vec_log0 is not None else None
            )

        self._assign_logical(vol_to_assign, spacing_to_assign)

    def _assign_logical(self, vol_log: np.ndarray, spacing_log: Tuple[float,float,float]):
        """Assigns new logical data and updates sliders."""
        try:
            self._overlay_cache.clear(); self._overlay_tag = None; self._ov_log = None
        except Exception:
            pass
        self._levels = self._levels0
        self._vol_log = vol_log
        self._spacing_log = spacing_log
        
        if self._vol_log is None:
             S, A, R = 0, 0, 0
        else:
            S, A, R = self._vol_log.shape
        
        self._cS, self._cA, self._cR = S//2, A//2, R//2
        self.pane_ax.sld.setEnabled(True); self.pane_ax.sld.setMinimum(-self._cS); self.pane_ax.sld.setMaximum((S-1)-self._cS)
        self.pane_co.sld.setEnabled(True); self.pane_co.sld.setMinimum(-self._cA); self.pane_co.sld.setMaximum((A-1)-self._cA)
        self.pane_sa.sld.setEnabled(True); self.pane_sa.sld.setMinimum(-self._cR); self.pane_sa.sld.setMaximum((R-1)-self._cR)
        
        # Clamp indices
        self._iS = max(0, min(S-1, self._iS))
        self._iA = max(0, min(A-1, self._iA))
        self._iR = max(0, min(R-1, self._iR))

        self._sync_sliders_from_state()
        self._update_triptych(); self._update_overlay_texts(); self._rescale_all()


    # ------------- DVF Loading & Management -------------
    
    def set_dvf_sources(self, dvf_map: Dict[str, str]):
        """Populates DVF dropdown in the Info Panel."""
        self._dvf_sources = dvf_map
        self._dvf_paths = {name: Path(p) for name, p in dvf_map.items()}

        # Preserve selection if possible, otherwise reset
        current_key = None # You could verify existing selection here
        self.dvf_info.update_dvf_list(dvf_map, current_key)

        # Auto vmax calculation
        self._auto_set_vmax_from_all_dvfs()
        self._update_dvf_legend()

    def _auto_set_vmax_from_all_dvfs(self):
        """Calculate global vmax from DVFs; cache per file (mtime/size) so polls stay cheap."""
        if not self._dvf_sources:
            self._dvf_vmax = 10.0
            self._dvf_vmax_file_cache.clear()
            return

        try:
            import SimpleITK as sitk
        except Exception:
            return

        active_keys: set[str] = set()
        global_max = 0.0

        for _, path_str in self._dvf_sources.items():
            try:
                path = Path(path_str)
                if not path.exists():
                    continue
                key = str(path.resolve())
                active_keys.add(key)
                st = path.stat()
                sig = (st.st_mtime_ns, st.st_size)
                cached = self._dvf_vmax_file_cache.get(key)
                if cached is not None and cached[0] == sig[0] and cached[1] == sig[1]:
                    global_max = max(global_max, cached[2])
                    continue

                img = sitk.ReadImage(str(path))
                comps = int(img.GetNumberOfComponentsPerPixel()) if hasattr(img, 'GetNumberOfComponentsPerPixel') else 1

                if comps > 1:
                    try:
                        mag = sitk.VectorMagnitude(img)
                    except Exception:
                        chans = [sitk.VectorIndexSelectionCast(img, c) for c in range(comps)]
                        mag = sitk.Sqrt(sum([sitk.Square(c) for c in chans]))
                else:
                    mag = sitk.Cast(img, sitk.sitkFloat32)

                arr = sitk.GetArrayViewFromImage(mag)
                m = float(np.nanmax(np.asarray(arr))) if arr.size else 0.0
                self._dvf_vmax_file_cache[key] = (sig[0], sig[1], m)
                global_max = max(global_max, m)
            except Exception:
                continue

        for k in list(self._dvf_vmax_file_cache.keys()):
            if k not in active_keys:
                del self._dvf_vmax_file_cache[k]

        if global_max > 0:
            self._dvf_vmax = nice_ceil_mm(global_max)
        else:
            self._dvf_vmax = 10.0

    def _on_dvf_phase_changed(self, index: int, key):
        """Called when user selects a DVF from the dropdown."""
        # Handle None selection (key is None or empty string due to signal type conversion)
        if not key:
            # None selected → remove overlay
            self._dvf_log0 = None
            self._dvf_log = None
            self._dvf_vec_log0 = None
            self._dvf_vec_log = None
            self.dvf_info.set_arrows_available(False)
            self.dvf_info.lbl_dvf_stats.setText("—")
            self._update_triptych()
            return
        
        path = self._dvf_paths.get(key)
        if not path: return
            
        try:
            import SimpleITK as sitk
            
            # 1) Read DVF
            img = sitk.ReadImage(str(path))
            comps = int(img.GetNumberOfComponentsPerPixel()) if hasattr(img, 'GetNumberOfComponentsPerPixel') else 1
            
            if comps > 1:
                try: mag = sitk.VectorMagnitude(img)
                except: 
                    chans = [sitk.VectorIndexSelectionCast(img, c) for c in range(comps)]
                    mag = sitk.Sqrt(sum([sitk.Square(c) for c in chans]))
            else:
                mag = sitk.Cast(img, sitk.sitkFloat32)

            mag = sitk.Cast(mag, sitk.sitkFloat32)

            # 2) Resample to CT geometry - this aligns DVF to CT's physical space
            if self._sitk_base_img is not None:
                mag = sitk.Resample(
                    mag, self._sitk_base_img, 
                    sitk.Transform(), sitk.sitkLinear, 
                    0.0, mag.GetPixelID()
                )

            # 3) Apply the SAME reorientation as CT (use stored params, don't recompute)
            # After resampling, DVF has same geometry as CT, so use CT's stored orientation
            if hasattr(self, '_sar_reorient_params') and self._sar_reorient_params:
                arr_zyx = sitk.GetArrayFromImage(mag).astype(np.float32)
                perm = self._sar_reorient_params['perm_zyx_to_SAR']
                flips = self._sar_reorient_params['flips_SAR']
                
                vol_SAR = np.transpose(arr_zyx, perm)
                if flips[0]: vol_SAR = vol_SAR[::-1,:,:]
                if flips[1]: vol_SAR = vol_SAR[:,::-1,:]
                if flips[2]: vol_SAR = vol_SAR[:,:,::-1]
            else:
                # Fallback: recompute (may cause offset)
                vol_SAR, _ = self._reorient_image_to_SAR_numpy(mag)
            
            # 4) Apply logical permutation
            if self._perm_phys_to_log is None:
                ov_log = vol_SAR
            else:
                ov_log = np.transpose(vol_SAR, self._perm_phys_to_log)

            self._dvf_log0 = ov_log.astype(np.float32)

            self._dvf_vec_log0 = None
            if comps >= 3 and self._sitk_base_img is not None:
                try:
                    layers: list[np.ndarray] = []
                    for ci in range(min(comps, 3)):
                        cimg = sitk.VectorIndexSelectionCast(img, ci)
                        m1 = sitk.Resample(
                            cimg,
                            self._sitk_base_img,
                            sitk.Transform(),
                            sitk.sitkLinear,
                            0.0,
                            cimg.GetPixelID(),
                        )
                        arr_zyx = sitk.GetArrayFromImage(m1).astype(np.float32)
                        if hasattr(self, "_sar_reorient_params") and self._sar_reorient_params:
                            ov_c = _dvf_scalar_zyx_to_log_volume(
                                arr_zyx, self._sar_reorient_params, self._perm_phys_to_log
                            )
                        else:
                            vol_SAR_c, _ = self._reorient_image_to_SAR_numpy(m1)
                            if self._perm_phys_to_log is None:
                                ov_c = vol_SAR_c
                            else:
                                ov_c = np.transpose(vol_SAR_c, self._perm_phys_to_log)
                        layers.append(ov_c.astype(np.float32))
                    if len(layers) == 3:
                        if self._dataset_type == "spare":
                            # Swap dz and dy components because axes S and A were swapped spatially
                            layers[0], layers[1] = layers[1], layers[0]
                        self._dvf_vec_log0 = np.stack(layers, axis=-1)
                except Exception:
                    self._dvf_vec_log0 = None

            self.dvf_info.set_arrows_available(self._dvf_vec_log0 is not None)

            # Apply current swap state
            self.apply_plane_swap(self._swap_active)

        except Exception as e:
            traceback.print_exc()
            self._dvf_log0 = None
            self._dvf_log = None
            self._dvf_vec_log0 = None
            self._dvf_vec_log = None
            self.dvf_info.set_arrows_available(False)
            self.dvf_info.lbl_dvf_stats.setText(f"Err: {e}")
            self._update_triptych()

    # ------------- Drawing & Hover -------------
    def _update_triptych(self):
        # Basic check for data
        if self._vol_log is None or self._levels is None:
            return

        iS, iA, iR = self._iS, self._iA, self._iR
        lo, hi = self._levels

        # Build per-pane slices using axis_index (same as parent CT viewer)
        view_tags = {0: "ax", 1: "co", 2: "sa"}  # for arrow helpers
        pane_list = [self.pane_ax, self.pane_co, self.pane_sa]
        pane_rgbs = {}  # axis_index -> rgb float array

        for pane in pane_list:
            idx = pane.axis_index
            name = self._axis_to_name[idx]
            slice_num = self._iS if name == "Axial" else (self._iA if name == "Coronal" else self._iR)

            try:
                if idx == 0:   slc = self._vol_log[slice_num, :, :]
                elif idx == 1: slc = self._vol_log[:, slice_num, :]
                else:          slc = self._vol_log[:, :, slice_num]
            except IndexError:
                continue

            img = np.flipud(np.fliplr(slc))

            # Apply pane-local transforms
            if pane.rotation_k != 0:
                img = np.rot90(img, k=pane.rotation_k)
            if pane.flip_h:
                img = np.fliplr(img)
            if pane.flip_v:
                img = np.flipud(img)

            rgb = np.repeat(_window_to_u8_levels(img, lo, hi)[..., None], 3, axis=2).astype(np.float32)
            pane_rgbs[idx] = rgb

        # Overlay DVF magnitude
        if self._dvf_log is not None and self._dvf_log.shape == self._vol_log.shape:
            vmax = float(self._dvf_vmax)
            alpha = float(self._dvf_alpha)

            # Stats from first pane (axial-labeled)
            first_stats_done = False

            def blend(rgb, dvf):
                norm = np.clip(dvf / max(vmax, 1e-6), 0.0, 1.0)
                colors = bgr_colormap(norm)
                mask = dvf > 0.0
                rgb[mask] = (1.0 - alpha) * rgb[mask] + alpha * colors[mask]

            for pane in pane_list:
                idx = pane.axis_index
                if idx not in pane_rgbs:
                    continue
                name = self._axis_to_name[idx]
                slice_num = self._iS if name == "Axial" else (self._iA if name == "Coronal" else self._iR)

                try:
                    if idx == 0:   dvf_slc = self._dvf_log[slice_num, :, :]
                    elif idx == 1: dvf_slc = self._dvf_log[:, slice_num, :]
                    else:          dvf_slc = self._dvf_log[:, :, slice_num]
                except IndexError:
                    continue

                dvf_slc = np.flipud(np.fliplr(dvf_slc))
                if pane.rotation_k != 0:
                    dvf_slc = np.rot90(dvf_slc, k=pane.rotation_k)
                if pane.flip_h:
                    dvf_slc = np.fliplr(dvf_slc)
                if pane.flip_v:
                    dvf_slc = np.flipud(dvf_slc)

                rgb = pane_rgbs[idx]
                if dvf_slc.shape == rgb.shape[:2]:
                    blend(rgb, dvf_slc)

                if not first_stats_done:
                    try:
                        self.dvf_info.lbl_dvf_stats.setText(
                            f"min {float(np.nanmin(dvf_slc)):.2f}  |  max {float(np.nanmax(dvf_slc)):.2f}  |  mean {float(np.nanmean(dvf_slc)):.2f} mm"
                        )
                    except Exception:
                        self.dvf_info.lbl_dvf_stats.setText("—")
                    first_stats_done = True

            # Arrows
            if (
                self.dvf_info.arrows_enabled()
                and self._dvf_vec_log is not None
                and self._dvf_vec_log.shape[:3] == self._vol_log.shape
                and self._dvf_vec_log.shape[-1] >= 3
                and self._spacing_log is not None
            ):
                perm = self._perm_phys_to_log
                sp = self._spacing_log
                arrow_kwargs = dict(
                    step=self.dvf_info.spn_arrow_step.value(),
                    length_scale=float(self.dvf_info.dbl_arrow_length.value()),
                    max_arrows=int(self.dvf_info.spn_arrow_max.value()),
                    min_mag_mm=float(self.dvf_info.dbl_arrow_min_mag.value()),
                )
                # Spacing pairs per axis index
                sp_pairs = {0: (sp[1], sp[2]), 1: (sp[0], sp[2]), 2: (sp[0], sp[1])}

                for pane in pane_list:
                    idx = pane.axis_index
                    if idx not in pane_rgbs:
                        continue
                    tag = view_tags[idx]
                    try:
                        u_arr, v_arr = _dvf_inplane_mm(self._dvf_vec_log, perm, iS, iA, iR, tag)
                        u_arr = np.flipud(np.fliplr(u_arr))
                        v_arr = np.flipud(np.fliplr(v_arr))
                        if pane.rotation_k != 0:
                            u_arr = np.rot90(u_arr, k=pane.rotation_k)
                            v_arr = np.rot90(v_arr, k=pane.rotation_k)
                        if pane.flip_h:
                            u_arr = np.fliplr(u_arr)
                            v_arr = np.fliplr(v_arr)
                        if pane.flip_v:
                            u_arr = np.flipud(u_arr)
                            v_arr = np.flipud(v_arr)
                        row_sp, col_sp = sp_pairs[idx]
                        _overlay_arrows_on_rgb(
                            pane_rgbs[idx], u_arr, v_arr,
                            row_sp, col_sp, **arrow_kwargs,
                        )
                    except Exception:
                        pass
        else:
            self.dvf_info.lbl_dvf_stats.setText("—")

        # Set pixmaps directly on pane labels (critical fix)
        for pane in pane_list:
            idx = pane.axis_index
            if idx in pane_rgbs:
                pane.lbl.setPixmap(_to_qpixmap_rgb(np.clip(pane_rgbs[idx], 0, 255).astype(np.uint8)))

        self._rescale_all()
        try: self._update_overlay_texts()
        except: pass

    def eventFilter(self, obj: QObject, event: QEvent) -> bool:
        if event.type() == QEvent.Type.MouseMove:
            for pane in (self.pane_ax, self.pane_co, self.pane_sa):
                if obj is pane.lbl:
                    self._update_hover_value(pane, event)
                    break
        return super().eventFilter(obj, event)

    def _update_hover_value(self, pane, e):
        if self._dvf_log is None or self._vol_log is None or self._dvf_log.shape != self._vol_log.shape: 
            return
            
        pm = pane.lbl.pixmap()
        if not pm or pm.isNull(): return
        w_disp, h_disp = pm.width(), pm.height()
        if w_disp <= 0 or h_disp <= 0: return
        x = max(0, min(w_disp-1, e.position().x()))
        y = max(0, min(h_disp-1, e.position().y()))
        iS, iA, iR = self._iS, self._iA, self._iR
        
        try:
            idx = pane.axis_index
            if idx == 0:
                H, W = self._vol_log.shape[1], self._vol_log.shape[2]
                dvf = np.flipud(np.fliplr(self._dvf_log[iS, :, :]))
            elif idx == 1:
                H, W = self._vol_log.shape[0], self._vol_log.shape[2]
                dvf = np.flipud(np.fliplr(self._dvf_log[:, iA, :]))
            else:
                H, W = self._vol_log.shape[0], self._vol_log.shape[1]
                dvf = np.flipud(np.fliplr(self._dvf_log[:, :, iR]))
            
            ix = int(x * W / w_disp); iy = int(y * H / h_disp)
            ix = max(0, min(W-1, ix)); iy = max(0, min(H-1, iy))

            val = float(dvf[iy, ix])
            for p in (self.pane_ax, self.pane_co, self.pane_sa):
                p.lbl.set_tl_text("")
            pane.lbl.set_tl_text(f"|DVF|={val:.2f} mm")
        except Exception:
            pass

    def _update_dvf_legend(self):
        try:
            w, h = 220, 14
            x = np.linspace(0.0, 1.0, w, dtype=np.float32)
            grad = np.tile(x[None, :], (h, 1))
            rgb = bgr_colormap(grad)
            pm = _to_qpixmap_rgb(rgb.astype(np.uint8))
            self.dvf_info.set_legend_pixmap(pm, f"0 → {self._dvf_vmax:.1f} mm")
        except Exception:
            pass

    def _on_alpha_changed(self, value: int):
        self._dvf_alpha = float(value) / 100.0
        self._update_triptych()

    def clear(self):
        super().clear() # Call base CTQuadPanel.clear()
        self._dvf_log0 = None
        self._dvf_log = None
        self._dvf_vec_log0 = None
        self._dvf_vec_log = None
        self._dvf_paths.clear()
        self._dvf_sources.clear()

        self.dvf_info.update_dvf_list({})
        self.dvf_info.lbl_dvf_stats.setText("—")
        self.dvf_info.chk_rebind.setChecked(False)
        self.dvf_info.sld_alpha.setValue(45)
        self.dvf_info.set_arrows_available(True)