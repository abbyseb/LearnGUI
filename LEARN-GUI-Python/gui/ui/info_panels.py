# ui/info_panels.py
from __future__ import annotations

from pathlib import Path

from PyQt6.QtCore import Qt, pyqtSignal, QSize
from PyQt6.QtGui import QPixmap
from PyQt6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSlider,
    QSpinBox,
    QVBoxLayout,
    QWidget,
)

class BaseInfoWidget(QGroupBox):
    """Base class for all Series Info panels to ensure consistent styling."""
    def __init__(self, title="Series Info"):
        super().__init__(title)
        self.layout = QFormLayout(self)
        self.layout.setVerticalSpacing(8)
        self.layout.setHorizontalSpacing(12)
        
        # Common labels
        self.lbl_source = QLabel("—")
        self.lbl_dims = QLabel("—")
        self.lbl_vox = QLabel("—")
        self.lbl_count = QLabel("—")
        
        for lbl in (self.lbl_source, self.lbl_dims, self.lbl_vox, self.lbl_count):
            lbl.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
            lbl.setWordWrap(True)

    def set_common_info(self, source="—", dims="—", vox="—", count="—"):
        self.lbl_source.setText(source)
        self.lbl_dims.setText(dims)
        self.lbl_vox.setText(vox)
        self.lbl_count.setText(count)


class CTInfoWidget(BaseInfoWidget):
    """
    Specific Info Panel for Planning CT.
    Contains: Phase Selector, Overlay Selector, Metadata, Flip Toggles.
    """
    phase_changed = pyqtSignal(int, str)    # index, text
    overlay_changed = pyqtSignal(str)       # overlay_name (or None)
    dicom_preview_series_uid_changed = pyqtSignal(str)  # "" = auto (largest), else SeriesInstanceUID

    def __init__(self):
        super().__init__("CT Series Info")
        self._in_preview_mode = False

        # --- 1. Phase Selector ---
        self.cmb_phase = QComboBox()
        self.cmb_phase.currentIndexChanged.connect(self._on_phase_emit)
        self.layout.addRow("Phase:", self.cmb_phase)

        # --- 2. Overlay Selector ---
        self.cmb_overlay = QComboBox()
        self.cmb_overlay.addItem("None", userData=None)
        self.cmb_overlay.currentIndexChanged.connect(self._on_overlay_emit)
        self.layout.addRow("Overlay:", self.cmb_overlay)

        # --- COPY step: multiple DICOM series under train/ ---
        self._lbl_preview_series = QLabel("DICOM series (copy):")
        self.cmb_preview_series = QComboBox()
        self.cmb_preview_series.setToolTip(
            "Several series can arrive during copy. Pick one to preview, or Auto for the largest."
        )
        self.cmb_preview_series.currentIndexChanged.connect(self._on_preview_series_pick)
        self.layout.addRow(self._lbl_preview_series, self.cmb_preview_series)
        self._lbl_preview_series.setVisible(False)
        self.cmb_preview_series.setVisible(False)

        # --- 3. Common Metadata ---
        self.layout.addRow("Source:", self.lbl_source)
        self.layout.addRow("Dims (z,y,x):", self.lbl_dims)
        self.layout.addRow("Voxel (z,y,x):", self.lbl_vox)
        self.layout.addRow("#Slices:", self.lbl_count)


    def _on_preview_series_pick(self, idx: int) -> None:
        if idx < 0:
            return
        data = self.cmb_preview_series.itemData(idx)
        uid = "" if data is None else str(data)
        self.dicom_preview_series_uid_changed.emit(uid)

    def update_dicom_preview_series(self, options: list[tuple[str, str]]) -> None:
        """COPY preview helper: (label, series_uid). Shown only when more than one series exists."""
        prev = None
        if self.cmb_preview_series.currentIndex() >= 0:
            prev = self.cmb_preview_series.currentData()
        self.cmb_preview_series.blockSignals(True)
        self.cmb_preview_series.clear()
        multi = len(options) > 1
        if multi:
            self.cmb_preview_series.addItem("Auto (largest)", userData="")
        for label, uid in options:
            self.cmb_preview_series.addItem(label, userData=uid)
        if multi and prev is not None:
            for i in range(self.cmb_preview_series.count()):
                if self.cmb_preview_series.itemData(i) == prev:
                    self.cmb_preview_series.setCurrentIndex(i)
                    break
        elif self.cmb_preview_series.count() > 0:
            self.cmb_preview_series.setCurrentIndex(0)
        self.cmb_preview_series.blockSignals(False)
        self._lbl_preview_series.setVisible(self._in_preview_mode and multi)
        self.cmb_preview_series.setVisible(self._in_preview_mode and multi)

    def clear_dicom_preview_series(self) -> None:
        self.cmb_preview_series.blockSignals(True)
        self.cmb_preview_series.clear()
        self.cmb_preview_series.blockSignals(False)
        self._lbl_preview_series.setVisible(False)
        self.cmb_preview_series.setVisible(False)

    def set_preview_mode(self, is_preview: bool):
        """
        If True (Step 1): Hides Phase, Overlay, Orientation/Rotation, and Flip controls.
        If False (Step 2+): Shows all controls.
        """
        self._in_preview_mode = is_preview
        multi = self.cmb_preview_series.count() > 1
        self.layout.setRowVisible(self.cmb_phase, not is_preview)
        self.layout.setRowVisible(self.cmb_overlay, not is_preview)
        self._lbl_preview_series.setVisible(is_preview and multi)
        self.cmb_preview_series.setVisible(is_preview and multi)

    def _on_phase_emit(self, idx):
        self.phase_changed.emit(idx, self.cmb_phase.itemText(idx))

    def _on_overlay_emit(self, idx):
        data = self.cmb_overlay.itemData(idx)
        self.overlay_changed.emit(data)

    def update_phase_list(self, names: list[str], current_index: int = 0):
        self.cmb_phase.blockSignals(True)
        self.cmb_phase.clear()
        self.cmb_phase.addItems(names)
        if 0 <= current_index < self.cmb_phase.count():
            self.cmb_phase.setCurrentIndex(current_index)
        self.cmb_phase.blockSignals(False)

    def update_overlay_list(self, overlay_names: list[str]):
        current_data = self.cmb_overlay.currentData()
        self.cmb_overlay.blockSignals(True)
        self.cmb_overlay.clear()
        self.cmb_overlay.addItem("None", userData=None)
        
        for name in sorted(overlay_names):
            self.cmb_overlay.addItem(name, userData=name)
            
        # Restore selection
        idx = self.cmb_overlay.findData(current_data)
        if idx >= 0:
            self.cmb_overlay.setCurrentIndex(idx)
            
        self.cmb_overlay.blockSignals(False)


class DVFInfoWidget(BaseInfoWidget):
    """
    Specific Info Panel for DVFs.
    Contains: DVF File Selector, Statistics, Scale Legend, Opacity, Rebind Toggle, Arrows.
    """
    dvf_changed = pyqtSignal(int, object)  # index, dvf_key (None or str)
    opacity_changed = pyqtSignal(int)   # 0-100
    rebind_changed = pyqtSignal(bool)   # checked
    arrow_settings_changed = pyqtSignal()

    def __init__(self):
        super().__init__("DVF Info")

        # --- 1. DVF Selector ---
        self.cmb_dvf = QComboBox()
        self.cmb_dvf.currentIndexChanged.connect(self._on_dvf_emit)
        self.layout.addRow("DVF Phase:", self.cmb_dvf)

        # --- 2. Metadata ---
        self.lbl_dvf_stats = QLabel("—")
        self.layout.addRow("DVF Slice Stats:", self.lbl_dvf_stats)

        # --- 3. Scale Legend ---
        self.lbl_scale_pic = QLabel()
        self.lbl_scale_text = QLabel("0 → 10 mm")
        scale_row = QWidget()
        sr = QHBoxLayout(scale_row)
        sr.setContentsMargins(0,0,0,0)
        sr.addWidget(self.lbl_scale_pic)
        sr.addWidget(self.lbl_scale_text)
        self.layout.addRow("Scale:", scale_row)

        # --- 4. Opacity Slider ---
        self.sld_alpha = QSlider(Qt.Orientation.Horizontal)
        self.sld_alpha.setRange(0, 100)
        self.sld_alpha.setValue(45)
        self.sld_alpha.valueChanged.connect(self.opacity_changed.emit)
        self.layout.addRow("Overlay Opacity:", self.sld_alpha)

        # --- 5. Rebind Toggle ---
        self.chk_rebind = QCheckBox("Rebind (Swap S↔A)")
        self.chk_rebind.setToolTip("Swap Superior/Axial dimensions")
        self.chk_rebind.stateChanged.connect(lambda: self.rebind_changed.emit(self.chk_rebind.isChecked()))
        self.layout.addRow("Plane:", self.chk_rebind)

        # --- 6. DVF arrows (vector field; scalar DVF files hide these) ---
        self.chk_arrows = QCheckBox("Show displacement arrows")
        self.chk_arrows.setToolTip("Requires 3-component DVF; in-plane part on each slice")
        self.chk_arrows.stateChanged.connect(self._emit_arrow_settings)
        self.layout.addRow("Arrows:", self.chk_arrows)

        self.spn_arrow_step = QSpinBox()
        self.spn_arrow_step.setRange(2, 64)
        self.spn_arrow_step.setValue(8)
        self.spn_arrow_step.setToolTip("Sample every N pixels along row/column")
        self.spn_arrow_step.valueChanged.connect(self._emit_arrow_settings)

        self.dbl_arrow_length = QDoubleSpinBox()
        self.dbl_arrow_length.setRange(0.25, 30.0)
        self.dbl_arrow_length.setSingleStep(0.25)
        self.dbl_arrow_length.setValue(3.0)
        self.dbl_arrow_length.setToolTip("Length scale (mm displacement → screen pixels)")
        self.dbl_arrow_length.valueChanged.connect(self._emit_arrow_settings)

        self.spn_arrow_max = QSpinBox()
        self.spn_arrow_max.setRange(200, 20000)
        self.spn_arrow_max.setSingleStep(200)
        self.spn_arrow_max.setValue(2500)
        self.spn_arrow_max.setToolTip("Max arrows drawn per view")
        self.spn_arrow_max.valueChanged.connect(self._emit_arrow_settings)

        self.dbl_arrow_min_mag = QDoubleSpinBox()
        self.dbl_arrow_min_mag.setRange(0.0, 5.0)
        self.dbl_arrow_min_mag.setSingleStep(0.05)
        self.dbl_arrow_min_mag.setValue(0.05)
        self.dbl_arrow_min_mag.setToolTip("Skip arrows where |DVF| is below this (mm)")
        self.dbl_arrow_min_mag.valueChanged.connect(self._emit_arrow_settings)

        self.layout.addRow("Arrow step (px):", self.spn_arrow_step)
        self.layout.addRow("Arrow length scale:", self.dbl_arrow_length)
        self.layout.addRow("Max arrows / view:", self.spn_arrow_max)
        self.layout.addRow("Min |DVF| (mm):", self.dbl_arrow_min_mag)

    def _emit_arrow_settings(self, *_args):
        self.arrow_settings_changed.emit()

    def _on_dvf_emit(self, idx):
        key = self.cmb_dvf.itemData(idx)
        self.dvf_changed.emit(idx, key)

    def arrows_enabled(self) -> bool:
        return self.chk_arrows.isChecked()

    def set_arrows_available(self, available: bool) -> None:
        """Scalar DVF: disable arrow controls."""
        for w in (
            self.chk_arrows,
            self.spn_arrow_step,
            self.dbl_arrow_length,
            self.spn_arrow_max,
            self.dbl_arrow_min_mag,
        ):
            w.setEnabled(bool(available))
        if not available:
            self.chk_arrows.blockSignals(True)
            self.chk_arrows.setChecked(False)
            self.chk_arrows.blockSignals(False)

    def update_dvf_list(self, dvf_map: dict, current_key=None):
        self.cmb_dvf.blockSignals(True)
        self.cmb_dvf.clear()
        self.cmb_dvf.addItem("None", userData=None)
        
        for name, path in sorted(dvf_map.items()):
            self.cmb_dvf.addItem(name, userData=name) # Using name as key
            
        if current_key:
            idx = self.cmb_dvf.findData(current_key)
            if idx >= 0:
                self.cmb_dvf.setCurrentIndex(idx)
        self.cmb_dvf.blockSignals(False)

    def set_legend_pixmap(self, pixmap: QPixmap, text: str):
        self.lbl_scale_pic.setPixmap(pixmap)
        self.lbl_scale_text.setText(text)


class DRRInfoWidget(BaseInfoWidget):
    """
    Specific Info Panel for DRR Cine.
    """
    ct_changed = pyqtSignal(int) # index

    def __init__(self):
        super().__init__("DRR Info")
        
        self.lbl_frame = QLabel("—")
        self.lbl_angle = QLabel("—")
        self.lbl_size = QLabel("—")
        
        self.layout.addRow("Frame:", self.lbl_frame)
        self.layout.addRow("Angle:", self.lbl_angle)
        self.layout.addRow("Size:", self.lbl_size)
        
        # CT Selector (Hidden unless multiple CTs)
        self.cmb_ct = QComboBox()
        self.cmb_ct.currentIndexChanged.connect(self.ct_changed.emit)
        self.lbl_ct_sel = QLabel("View CT:")
        
        self.layout.addRow(self.lbl_ct_sel, self.cmb_ct)
        self.lbl_ct_sel.setVisible(False)
        self.cmb_ct.setVisible(False)

    def get_drr_generation_options(self) -> dict:
        """Defaults for modules.drr_generation.generate.generate_drrs (matches former panel values)."""
        _bundled_xml = (
            Path(__file__).resolve().parent.parent.parent
            / "modules"
            / "drr_generation"
            / "RTKGeometry_360.xml"
        )
        return {
            "geometry_path": _bundled_xml,
            "geometry_source": "circular",
            "circular_sid_mm": 1000.0,
            "circular_sdd_mm": 1500.0,
            "circular_first_angle_deg": 0.0,
            "circular_angle_step_deg": 3.0,
            "circular_num_projections": 120,
            "detector_origin": (-200.0, -150.0, 0.0),
            "detector_spacing": (0.388, 0.388, 1.0),
            "detector_size_xy": (1024, 768),
        }

    def update_ct_list(self, ct_names: list, user_data: list):
        self.cmb_ct.blockSignals(True)
        self.cmb_ct.clear()
        for name, data in zip(ct_names, user_data):
            self.cmb_ct.addItem(name, userData=data)
        
        has_multi = len(ct_names) > 1
        self.cmb_ct.setVisible(has_multi)
        self.lbl_ct_sel.setVisible(has_multi)
        self.cmb_ct.blockSignals(False)

    def set_status(self, frame_str, angle_str, size_str):
        self.lbl_frame.setText(frame_str)
        self.lbl_angle.setText(angle_str)
        self.lbl_size.setText(size_str)