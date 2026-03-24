# ui/info_panels.py
from __future__ import annotations
from PyQt6.QtCore import Qt, pyqtSignal, QSize
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QFormLayout, QLabel, QComboBox, 
    QCheckBox, QHBoxLayout, QGroupBox, QSlider
)
from PyQt6.QtGui import QPixmap

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
    flip_changed = pyqtSignal()             # Emitted when any flip toggle changes

    def __init__(self):
        super().__init__("CT Series Info")
        
        # --- 1. Phase Selector ---
        self.cmb_phase = QComboBox()
        self.cmb_phase.currentIndexChanged.connect(self._on_phase_emit)
        self.layout.addRow("Phase:", self.cmb_phase)

        # --- 2. Overlay Selector ---
        self.cmb_overlay = QComboBox()
        self.cmb_overlay.addItem("None", userData=None)
        self.cmb_overlay.currentIndexChanged.connect(self._on_overlay_emit)
        self.layout.addRow("Overlay:", self.cmb_overlay)

        # --- 3. Common Metadata ---
        self.layout.addRow("Source:", self.lbl_source)
        self.layout.addRow("Dims (z,y,x):", self.lbl_dims)
        self.layout.addRow("Voxel (z,y,x):", self.lbl_vox)
        self.layout.addRow("#Slices:", self.lbl_count)

        # --- 4. Flip Toggles ---
        self.flip_container = QWidget()
        flip_lay = QHBoxLayout(self.flip_container)
        flip_lay.setContentsMargins(0, 5, 0, 0)
        
        self.chk_flip_cor = QCheckBox("Coronal Flip")
        self.chk_flip_ax = QCheckBox("Axial Flip")
        self.chk_flip_sag = QCheckBox("Sagittal Flip")
        
        for chk in (self.chk_flip_cor, self.chk_flip_ax, self.chk_flip_sag):
            chk.stateChanged.connect(lambda: self.flip_changed.emit())
            flip_lay.addWidget(chk)
        
        self.layout.addRow(self.flip_container)

    def set_preview_mode(self, is_preview: bool):
        """
        If True (Step 1): Hides Phase, Overlay, and Flip controls.
        If False (Step 2+): Shows all controls.
        """
        # QFormLayout.setRowVisible takes the widget in the field position
        self.layout.setRowVisible(self.cmb_phase, not is_preview)
        self.layout.setRowVisible(self.cmb_overlay, not is_preview)
        self.layout.setRowVisible(self.flip_container, not is_preview)

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
    Contains: DVF File Selector, Statistics, Scale Legend, Opacity, Rebind Toggle.
    """
    dvf_changed = pyqtSignal(int, object)  # index, dvf_key (None or str)
    opacity_changed = pyqtSignal(int)   # 0-100
    rebind_changed = pyqtSignal(bool)   # checked

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

    def _on_dvf_emit(self, idx):
        key = self.cmb_dvf.itemData(idx)
        self.dvf_changed.emit(idx, key)

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