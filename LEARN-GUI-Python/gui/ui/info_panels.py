# ui/info_panels.py
from __future__ import annotations

from pathlib import Path

from PyQt6.QtCore import Qt, pyqtSignal, QSize
from PyQt6.QtGui import QPixmap
from PyQt6.QtWidgets import (
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
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
    flip_changed = pyqtSignal()             # Emitted when any flip toggle changes
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

        # --- 4. Flip Toggles ---
        self.flip_container = QWidget()
        flip_lay = QHBoxLayout(self.flip_container)
        flip_lay.setContentsMargins(0, 5, 0, 0)

        self.chk_flip_cor = QCheckBox("Coronal Flip")
        self.chk_flip_ax = QCheckBox("Axial Flip")
        self.chk_flip_sag = QCheckBox("Sagittal Flip")
        flip_lay.addWidget(self.chk_flip_cor)
        flip_lay.addWidget(self.chk_flip_ax)
        flip_lay.addWidget(self.chk_flip_sag)
        self.chk_flip_cor.stateChanged.connect(lambda *_: self.flip_changed.emit())
        self.chk_flip_ax.stateChanged.connect(lambda *_: self.flip_changed.emit())
        self.chk_flip_sag.stateChanged.connect(lambda *_: self.flip_changed.emit())
        self.layout.addRow("Flip:", self.flip_container)

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
        If True (Step 1): Hides Phase, Overlay, and Flip controls.
        If False (Step 2+): Shows all controls.
        """
        self._in_preview_mode = is_preview
        multi = self.cmb_preview_series.count() > 1
        self.layout.setRowVisible(self.cmb_phase, not is_preview)
        self.layout.setRowVisible(self.cmb_overlay, not is_preview)
        self.layout.setRowVisible(self.flip_container, not is_preview)
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

        # --- DRR generation (used on next pipeline DRR step) ---
        self._grp_drr_gen = QGroupBox("DRR setup (next run)")
        self._grp_drr_gen.setStyleSheet(
            "QGroupBox { font-size: 10pt; } QGroupBox::title { subcontrol-origin: margin; left: 8px; }"
        )
        gf = QFormLayout(self._grp_drr_gen)
        gf.setVerticalSpacing(6)

        self.cmb_geom_source = QComboBox()
        self.cmb_geom_source.addItem("XML file (exact shipped matrices)", "xml")
        self.cmb_geom_source.addItem("Circular orbit (SID / SDD / angles)", "circular")
        self.cmb_geom_source.currentIndexChanged.connect(lambda *_: self._sync_drr_geom_widgets())
        gf.addRow("Geometry:", self.cmb_geom_source)

        hl_xml = QHBoxLayout()
        self._ed_geom_xml = QLineEdit()
        _def_xml = (
            Path(__file__).resolve().parent.parent.parent
            / "modules"
            / "drr_generation"
            / "RTKGeometry_360.xml"
        )
        self._ed_geom_xml.setText(str(_def_xml))
        self._btn_geom_xml = QPushButton("Browse…")
        self._btn_geom_xml.clicked.connect(self._browse_geom_xml)
        hl_xml.addWidget(self._ed_geom_xml, 1)
        hl_xml.addWidget(self._btn_geom_xml)
        gf.addRow("XML path:", hl_xml)

        self._spin_sid = QDoubleSpinBox()
        self._spin_sid.setRange(100.0, 3000.0)
        self._spin_sid.setDecimals(1)
        self._spin_sid.setValue(1000.0)
        self._spin_sid.setSuffix(" mm")
        self._spin_sid.setToolTip("Source-to-isocenter distance (matches XML tag of the same name).")
        gf.addRow("SID:", self._spin_sid)

        self._spin_sdd = QDoubleSpinBox()
        self._spin_sdd.setRange(100.0, 4000.0)
        self._spin_sdd.setDecimals(1)
        self._spin_sdd.setValue(1500.0)
        self._spin_sdd.setSuffix(" mm")
        self._spin_sdd.setToolTip("Source-to-detector distance.")
        gf.addRow("SDD:", self._spin_sdd)

        self._spin_first_ang = QDoubleSpinBox()
        self._spin_first_ang.setRange(-360.0, 360.0)
        self._spin_first_ang.setDecimals(2)
        self._spin_first_ang.setValue(0.0)
        self._spin_first_ang.setSuffix("°")
        gf.addRow("First gantry °:", self._spin_first_ang)

        self._spin_step_ang = QDoubleSpinBox()
        self._spin_step_ang.setRange(0.1, 90.0)
        self._spin_step_ang.setDecimals(3)
        self._spin_step_ang.setValue(3.0)
        self._spin_step_ang.setSuffix("°")
        gf.addRow("Angle step:", self._spin_step_ang)

        self._spin_nproj = QSpinBox()
        self._spin_nproj.setRange(1, 720)
        self._spin_nproj.setValue(120)
        self._spin_nproj.setToolTip("Number of projections (circular mode). XML mode ignores this.")
        gf.addRow("# projections:", self._spin_nproj)

        self._spin_det_ox = QDoubleSpinBox()
        self._spin_det_oy = QDoubleSpinBox()
        self._spin_det_oz = QDoubleSpinBox()
        for sp, v in ((self._spin_det_ox, -200.0), (self._spin_det_oy, -150.0), (self._spin_det_oz, 0.0)):
            sp.setRange(-2000.0, 2000.0)
            sp.setDecimals(3)
            sp.setValue(v)
            sp.setSuffix(" mm")
        hl_o = QHBoxLayout()
        hl_o.addWidget(QLabel("X"))
        hl_o.addWidget(self._spin_det_ox)
        hl_o.addWidget(QLabel("Y"))
        hl_o.addWidget(self._spin_det_oy)
        hl_o.addWidget(QLabel("Z"))
        hl_o.addWidget(self._spin_det_oz)
        gf.addRow("Detector origin:", hl_o)

        self._spin_det_sx = QDoubleSpinBox()
        self._spin_det_sy = QDoubleSpinBox()
        self._spin_det_sz = QDoubleSpinBox()
        for sp, v in ((self._spin_det_sx, 0.388), (self._spin_det_sy, 0.388), (self._spin_det_sz, 1.0)):
            sp.setRange(0.001, 50.0)
            sp.setDecimals(4)
            sp.setValue(v)
            sp.setSuffix(" mm")
        hl_s = QHBoxLayout()
        hl_s.addWidget(QLabel("dX"))
        hl_s.addWidget(self._spin_det_sx)
        hl_s.addWidget(QLabel("dY"))
        hl_s.addWidget(self._spin_det_sy)
        hl_s.addWidget(QLabel("dZ"))
        hl_s.addWidget(self._spin_det_sz)
        gf.addRow("Detector spacing:", hl_s)

        self._spin_det_nx = QSpinBox()
        self._spin_det_ny = QSpinBox()
        self._spin_det_nx.setRange(64, 4096)
        self._spin_det_ny.setRange(64, 4096)
        self._spin_det_nx.setValue(1024)
        self._spin_det_ny.setValue(768)
        hl_n = QHBoxLayout()
        hl_n.addWidget(QLabel("W"))
        hl_n.addWidget(self._spin_det_nx)
        hl_n.addWidget(QLabel("H"))
        hl_n.addWidget(self._spin_det_ny)
        gf.addRow("Detector pixels:", hl_n)

        tip = QLabel(
            "XML = use precomputed projection matrices (same as forward_project.py). "
            "Circular = RTK AddProjection with your SID/SDD and angle list. "
            "Detector block maps to ConstantImageSource (how the detector grid sits in mm)."
        )
        tip.setWordWrap(True)
        tip.setStyleSheet("color: #888; font-size: 9pt;")
        gf.addRow(tip)

        self.layout.addRow(self._grp_drr_gen)

        self._circular_widgets = [
            self._spin_sid,
            self._spin_sdd,
            self._spin_first_ang,
            self._spin_step_ang,
            self._spin_nproj,
        ]
        self._xml_widgets = [self._ed_geom_xml, self._btn_geom_xml]
        self._sync_drr_geom_widgets()

    def _sync_drr_geom_widgets(self) -> None:
        use_xml = self.cmb_geom_source.currentData() == "xml"
        for w in self._xml_widgets:
            w.setEnabled(use_xml)
        for w in self._circular_widgets:
            w.setEnabled(not use_xml)

    def _browse_geom_xml(self) -> None:
        p, _ = QFileDialog.getOpenFileName(
            self,
            "RTK geometry XML",
            str(Path(self._ed_geom_xml.text()).parent),
            "XML (*.xml);;All (*)",
        )
        if p:
            self._ed_geom_xml.setText(p)

    def get_drr_generation_options(self) -> dict:
        """Kwargs fragment for modules.drr_generation.generate.generate_drrs (after train_dir)."""
        src = self.cmb_geom_source.currentData()
        return {
            "geometry_path": Path(self._ed_geom_xml.text().strip()),
            "geometry_source": src if src else "xml",
            "circular_sid_mm": float(self._spin_sid.value()),
            "circular_sdd_mm": float(self._spin_sdd.value()),
            "circular_first_angle_deg": float(self._spin_first_ang.value()),
            "circular_angle_step_deg": float(self._spin_step_ang.value()),
            "circular_num_projections": int(self._spin_nproj.value()),
            "detector_origin": (
                float(self._spin_det_ox.value()),
                float(self._spin_det_oy.value()),
                float(self._spin_det_oz.value()),
            ),
            "detector_spacing": (
                float(self._spin_det_sx.value()),
                float(self._spin_det_sy.value()),
                float(self._spin_det_sz.value()),
            ),
            "detector_size_xy": (
                int(self._spin_det_nx.value()),
                int(self._spin_det_ny.value()),
            ),
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