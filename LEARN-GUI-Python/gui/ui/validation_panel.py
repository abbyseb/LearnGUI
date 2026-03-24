# ui/validation_panel.py
"""
DVF Validation Panel - Matches VoxelMap interface style.
Validates DVF accuracy using fiducial marker TRE analysis.

Uses same slice extraction logic as CTQuadPanel:
- Volume in SAR orientation (Superior-Anterior-Right)
- Shape: (S, A, R)
- Axial:    vol[iS, :, :] → rows=A, cols=R
- Coronal:  vol[:, iA, :] → rows=S, cols=R  
- Sagittal: vol[:, :, iR] → rows=S, cols=A
- All slices have flipud(fliplr(...)) applied
"""
from __future__ import annotations
import numpy as np
from typing import List, Optional, Tuple
from dataclasses import dataclass
from pathlib import Path

from PyQt6.QtCore import Qt, pyqtSignal, QTimer
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QGroupBox,
    QLabel, QComboBox, QTableWidget, QTableWidgetItem, QPushButton,
    QFormLayout, QHeaderView, QSizePolicy, QSlider, QSplitter
)
from PyQt6.QtGui import QPixmap, QImage, QPainter, QColor, QFont

# =============================================================================
# DATA STRUCTURES
# =============================================================================

@dataclass
class MarkerPosition:
    """3D position of a fiducial marker (mm)."""
    x: float  # LR (Right positive)
    y: float  # AP (Anterior positive)
    z: float  # SI (Superior positive)

    def to_array(self) -> np.ndarray:
        return np.array([self.x, self.y, self.z])

    @classmethod
    def from_array(cls, arr: np.ndarray) -> 'MarkerPosition':
        return cls(x=arr[0], y=arr[1], z=arr[2])


@dataclass
class ValidationResult:
    """Single validation measurement."""
    marker_id: int
    phase: str
    fraction: str
    predicted: MarkerPosition
    ground_truth: MarkerPosition
    tre: float
    error_lr: float
    error_ap: float
    error_si: float


# =============================================================================
# DEMO DATA
# =============================================================================

class DemoDataGenerator:
    """Generates realistic synthetic validation data."""

    def __init__(self, seed: int = 42):
        np.random.seed(seed)
        # Marker positions relative to volume center (in mm)
        # These will appear in different locations in the CT
        self.marker_refs = {
            1: MarkerPosition(x=20.0, y=-15.0, z=10.0),   # Right, posterior, superior
            2: MarkerPosition(x=-25.0, y=-20.0, z=-5.0),  # Left, posterior, inferior
            3: MarkerPosition(x=0.0, y=-30.0, z=0.0),     # Center, posterior, mid
        }
        self.phases = [f"CT_{i:02d}" for i in range(1, 11)]
        self.fractions = [f"Fx{i:02d}" for i in range(1, 6)]

    def generate_breathing_motion(self, phase_idx: int) -> np.ndarray:
        """Generate breathing motion. Phase 6 is exhale (reference)."""
        t = (phase_idx - 1) / 9.0
        si = 15.0 * np.sin(np.pi * t)
        ap = 5.0 * np.sin(np.pi * t + 0.2)
        lr = 1.5 * np.sin(2 * np.pi * t)
        return np.array([lr, ap, si])

    def generate_validation_results(self) -> List[ValidationResult]:
        results = []
        for fx in self.fractions:
            for marker_id, ref_pos in self.marker_refs.items():
                for phase_idx, phase in enumerate(self.phases, 1):
                    true_motion = self.generate_breathing_motion(phase_idx)
                    # Error increases away from reference phase (CT_06)
                    phase_factor = abs(phase_idx - 6) / 5.0
                    base_error = 0.5 + 2.0 * phase_factor
                    prediction_error = np.random.normal(0, base_error, 3)
                    prediction_error *= np.array([0.5, 0.8, 1.0])  # SI has most error
                    gt_pos = ref_pos.to_array() + true_motion
                    pred_pos = gt_pos + prediction_error
                    tre = np.linalg.norm(prediction_error)
                    results.append(ValidationResult(
                        marker_id=marker_id, phase=phase, fraction=fx,
                        predicted=MarkerPosition.from_array(pred_pos),
                        ground_truth=MarkerPosition.from_array(gt_pos),
                        tre=tre, error_lr=prediction_error[0],
                        error_ap=prediction_error[1], error_si=prediction_error[2],
                    ))
        return results


class ValidationStatistics:
    """Computes statistics from validation results."""

    def __init__(self, results: List[ValidationResult]):
        self.results = results
        tres = np.array([r.tre for r in results])

        self.overall = {
            'mean': np.mean(tres), 'std': np.std(tres),
            'median': np.median(tres), 'p95': np.percentile(tres, 95),
            'max': np.max(tres), 'n': len(tres),
        }

        self.per_marker = {}
        for m in set(r.marker_id for r in results):
            mt = [r.tre for r in results if r.marker_id == m]
            self.per_marker[m] = {'mean': np.mean(mt), 'std': np.std(mt), 'median': np.median(mt)}

        self.per_direction = {
            'LR': {'mean': np.mean([abs(r.error_lr) for r in results]), 'std': np.std([r.error_lr for r in results])},
            'AP': {'mean': np.mean([abs(r.error_ap) for r in results]), 'std': np.std([r.error_ap for r in results])},
            'SI': {'mean': np.mean([abs(r.error_si) for r in results]), 'std': np.std([r.error_si for r in results])},
        }


# =============================================================================
# CT VIEWER WITH MARKER OVERLAY (matches CTQuadPanel)
# =============================================================================

class OverlayLabel(QLabel):
    """QLabel with corner text overlays (matches panels.py)."""
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
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)
        m = 6
        # Draw text with shadow for visibility
        for txt, align, adj in [
            (self._tl, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop, (6, 4, 0, 0)),
            (self._bl, Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignBottom, (m, 0, -m, -4)),
            (self._br, Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignBottom, (m, 0, -m, -4)),
        ]:
            if txt:
                rect = self.rect().adjusted(*adj)
                # Shadow
                p.setPen(QColor(0, 0, 0, 220))
                p.drawText(rect.adjusted(1, 1, 1, 1), align, txt)
                # Text
                p.setPen(QColor(255, 255, 255))
                p.drawText(rect, align, txt)
        p.end()


class MarkerCTViewer(QGroupBox):
    """
    CT triptych viewer with marker overlay.
    Uses same slice extraction logic as CTQuadPanel.
    """
    def __init__(self, title: str = "Geometric Verification"):
        super().__init__(title)

        self.grid = QGridLayout(self)
        self.grid.setContentsMargins(4, 4, 4, 4)
        self.grid.setSpacing(4)

        # Create view labels (same layout as CTQuadPanel triptych)
        # Top-left: Coronal, Top-right: Axial, Bottom-left: Sagittal
        self.lbl_co = OverlayLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self.lbl_ax = OverlayLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self.lbl_sa = OverlayLabel(alignment=Qt.AlignmentFlag.AlignCenter)

        for lbl, name in [(self.lbl_co, "Coronal"), (self.lbl_ax, "Axial"), (self.lbl_sa, "Sagittal")]:
            lbl.setStyleSheet("background-color:#000; border:0; border-radius:0;")
            lbl.setMinimumSize(24, 24)
            lbl.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
            lbl.set_bl_text(name)

        # Sliders (center-relative like CTQuadPanel)
        self.sld_ax = QSlider(Qt.Orientation.Horizontal)  # Controls S index (axial position)
        self.sld_co = QSlider(Qt.Orientation.Horizontal)  # Controls A index (coronal position)
        self.sld_sa = QSlider(Qt.Orientation.Horizontal)  # Controls R index (sagittal position)
        for sld in [self.sld_ax, self.sld_co, self.sld_sa]:
            sld.setMinimum(0)
            sld.setMaximum(100)
            sld.setValue(0)
            sld.valueChanged.connect(self._on_slider)

        # Wrap each view with slider
        def wrap(lbl, sld):
            w = QWidget()
            lay = QVBoxLayout(w)
            lay.setContentsMargins(0, 0, 0, 0)
            lay.addWidget(lbl, 1)
            lay.addWidget(sld)
            return w

        co_wrap = wrap(self.lbl_co, self.sld_co)
        ax_wrap = wrap(self.lbl_ax, self.sld_ax)
        sa_wrap = wrap(self.lbl_sa, self.sld_sa)

        # Info panel (bottom-right)
        self.info_panel = QGroupBox("Marker Details")
        info_layout = QFormLayout(self.info_panel)
        info_layout.setVerticalSpacing(6)

        self.lbl_marker = QLabel("—")
        self.lbl_phase = QLabel("—")
        self.lbl_tre = QLabel("—")
        self.lbl_tre.setStyleSheet("font-weight: bold; font-size: 14px;")

        info_layout.addRow("Marker:", self.lbl_marker)
        info_layout.addRow("Phase:", self.lbl_phase)
        info_layout.addRow("TRE:", self.lbl_tre)

        # Legend
        legend = QWidget()
        leg_lay = QVBoxLayout(legend)
        leg_lay.setContentsMargins(0, 8, 0, 0)
        leg_lay.setSpacing(4)

        gt_row = QHBoxLayout()
        gt_icon = QLabel("●")
        gt_icon.setStyleSheet("color: #00ff00; font-size: 14px;")
        gt_row.addWidget(gt_icon)
        gt_row.addWidget(QLabel("Ground Truth"))
        gt_row.addStretch()
        leg_lay.addLayout(gt_row)

        pred_row = QHBoxLayout()
        pred_icon = QLabel("×")
        pred_icon.setStyleSheet("color: #ff6464; font-size: 16px; font-weight: bold;")
        pred_row.addWidget(pred_icon)
        pred_row.addWidget(QLabel("Prediction"))
        pred_row.addStretch()
        leg_lay.addLayout(pred_row)

        info_layout.addRow(legend)

        # Grid layout (matches CTQuadPanel triptych)
        self.grid.addWidget(co_wrap, 0, 0)       # Coronal top-left
        self.grid.addWidget(ax_wrap, 0, 1)       # Axial top-right
        self.grid.addWidget(sa_wrap, 1, 0)       # Sagittal bottom-left
        self.grid.addWidget(self.info_panel, 1, 1)  # Info bottom-right

        self.grid.setColumnStretch(0, 1)
        self.grid.setColumnStretch(1, 1)
        self.grid.setRowStretch(0, 1)
        self.grid.setRowStretch(1, 1)

        # Volume state (SAR orientation)
        self._vol_SAR: Optional[np.ndarray] = None
        self._spacing_SAR: Tuple[float, float, float] = (2.0, 1.0, 1.0)  # (s_mm, a_mm, r_mm)
        self._levels: Tuple[float, float] = (-600, 400)

        # Slice indices (center-relative)
        self._cS = self._cA = self._cR = 0
        self._iS = self._iA = self._iR = 0

        # Pixmap cache
        self._pm_ax: Optional[QPixmap] = None
        self._pm_co: Optional[QPixmap] = None
        self._pm_sa: Optional[QPixmap] = None

        # Marker state
        self._predicted: Optional[MarkerPosition] = None
        self._ground_truth: Optional[MarkerPosition] = None

        self._load_demo_volume()

    def _load_demo_volume(self):
        """Load real CT volume or fall back to synthetic."""
        # Try to load real CT (platform-specific example paths)
        real_paths = [
            Path(r"C:\Users\amac2328\Desktop\VALKIM_01_Run_20260121_000842\Patient01\intermediates\CT_01.mha"),
            Path("/mnt/user-data/uploads/CT_01.mha"),
            Path("/Volumes/PRJ-LEARN/LEARN/GUI/Runs/VALKIM_01_Run_20260121_000842/Patient01/intermediates/CT_01.mha"),
        ]
        
        for p in real_paths:
            if p.exists():
                try:
                    import SimpleITK as sitk
                    img = sitk.ReadImage(str(p))
                    vol, spacing = self._reorient_to_SAR(img)
                    self._vol_SAR = vol
                    self._spacing_SAR = spacing
                    self._levels = self._compute_levels(vol)
                    break
                except Exception as e:
                    print(f"Failed to load {p}: {e}")
        
        # Fall back to synthetic if no real CT loaded
        if self._vol_SAR is None:
            self._create_synthetic_volume()

        # Set up indices
        S, A, R = self._vol_SAR.shape
        self._cS, self._cA, self._cR = S // 2, A // 2, R // 2
        self._iS, self._iA, self._iR = self._cS, self._cA, self._cR

        # Configure sliders (center-relative)
        self.sld_ax.setMinimum(-self._cS); self.sld_ax.setMaximum(S - 1 - self._cS); self.sld_ax.setValue(0)
        self.sld_co.setMinimum(-self._cA); self.sld_co.setMaximum(A - 1 - self._cA); self.sld_co.setValue(0)
        self.sld_sa.setMinimum(-self._cR); self.sld_sa.setMaximum(R - 1 - self._cR); self.sld_sa.setValue(0)

        self._update_triptych()
        QTimer.singleShot(50, self._rescale_all)

    def _reorient_to_SAR(self, img):
        """Reorient SimpleITK image to SAR (same logic as CTQuadPanel)."""
        import SimpleITK as sitk
        arr_zyx = sitk.GetArrayFromImage(img).astype(np.float32)
        D_lps = np.array(img.GetDirection(), dtype=float).reshape(3, 3)
        L2R = np.diag([-1.0, -1.0, 1.0])
        D_ras = L2R @ D_lps
        
        axes = ['I', 'J', 'K']; anat = ['R', 'A', 'S']
        idx_for = {}; sign_for = {}; used = set()
        for t_idx, t_lbl in enumerate(anat):
            best_axis = None; best_val = -1.0; best_sign = +1
            for col, name in enumerate(axes):
                if name in used: continue
                v = D_ras[:, col]; val = abs(v[t_idx])
                if val > best_val:
                    best_val = val; best_axis = name; best_sign = +1 if v[t_idx] >= 0 else -1
            idx_for[t_lbl] = best_axis; sign_for[t_lbl] = best_sign; used.add(best_axis)
        
        name_to_npaxis = {'K': 0, 'J': 1, 'I': 2}
        perm = (name_to_npaxis[idx_for['S']], name_to_npaxis[idx_for['A']], name_to_npaxis[idx_for['R']])
        vol = np.transpose(arr_zyx, perm)
        if sign_for['S'] < 0: vol = vol[::-1, :, :]
        if sign_for['A'] < 0: vol = vol[:, ::-1, :]
        if sign_for['R'] < 0: vol = vol[:, :, ::-1]
        
        sx, sy, sz = img.GetSpacing()
        spacing_map = {'I': sx, 'J': sy, 'K': sz}
        spacing_SAR = (spacing_map[idx_for['S']], spacing_map[idx_for['A']], spacing_map[idx_for['R']])
        return vol, spacing_SAR

    def _compute_levels(self, vol):
        """Compute window levels from volume."""
        finite = np.isfinite(vol)
        if finite.any():
            lo, hi = np.percentile(vol[finite], [0.5, 99.5])
        else:
            lo, hi = float(vol.min()), float(vol.max())
        if hi <= lo: hi = lo + 1.0
        return (lo, hi)

    def _create_synthetic_volume(self):
        """Create synthetic CT volume as fallback."""
        self._spacing_SAR = (2.0, 1.0, 1.0)
        nS, nA, nR = 100, 256, 256
        vol = np.zeros((nS, nA, nR), dtype=np.float32)
        vol[:] = -1000
        
        cA, cR = nA // 2, nR // 2
        for si in range(nS):
            for ai in range(nA):
                for ri in range(nR):
                    if ((ri - cR) / 90) ** 2 + ((ai - cA) / 70) ** 2 <= 1:
                        vol[si, ai, ri] = 40
                    if ((ri - cR + 40) / 35) ** 2 + ((ai - cA) / 50) ** 2 <= 1:
                        vol[si, ai, ri] = -700
                    if ((ri - cR - 40) / 35) ** 2 + ((ai - cA) / 50) ** 2 <= 1:
                        vol[si, ai, ri] = -700
                    if ((ri - cR) / 15) ** 2 + ((ai - cA - 50) / 12) ** 2 <= 1:
                        vol[si, ai, ri] = 400
        
        self._vol_SAR = vol
        self._levels = (-600, 400)

    def _on_slider(self, _val=None):
        if self._vol_SAR is None: return
        S, A, R = self._vol_SAR.shape

        self._iS = max(0, min(S - 1, self._cS + self.sld_ax.value()))
        self._iA = max(0, min(A - 1, self._cA + self.sld_co.value()))
        self._iR = max(0, min(R - 1, self._cR + self.sld_sa.value()))

        self._update_triptych()

    def set_result(self, result: ValidationResult):
        """Display validation result with markers."""
        self._predicted = result.predicted
        self._ground_truth = result.ground_truth

        self.lbl_marker.setText(f"Fiducial {result.marker_id}")
        self.lbl_phase.setText(result.phase)

        color = "#64c864" if result.tre < 2.0 else "#ffc864" if result.tre < 3.0 else "#ff6464"
        self.lbl_tre.setText(f"<span style='color:{color}'>{result.tre:.2f} mm</span>")

        self._update_triptych()

    def _update_triptych(self):
        """Extract slices and render (matches CTQuadPanel._update_triptych)."""
        if self._vol_SAR is None: return

        iS, iA, iR = self._iS, self._iA, self._iR
        lo, hi = self._levels

        # Extract slices with flipud/fliplr (same as CTQuadPanel)
        ax_slice = np.flipud(np.fliplr(self._vol_SAR[iS, :, :]))  # rows=A, cols=R
        co_slice = np.flipud(np.fliplr(self._vol_SAR[:, iA, :]))  # rows=S, cols=R
        sa_slice = np.flipud(np.fliplr(self._vol_SAR[:, :, iR]))  # rows=S, cols=A

        # Window to u8
        def to_u8(arr):
            return np.clip((arr - lo) / max(1e-6, hi - lo) * 255, 0, 255).astype(np.uint8)

        ax_u8 = to_u8(ax_slice)
        co_u8 = to_u8(co_slice)
        sa_u8 = to_u8(sa_slice)

        # Convert to RGB
        ax_rgb = np.stack([ax_u8, ax_u8, ax_u8], axis=-1).copy()
        co_rgb = np.stack([co_u8, co_u8, co_u8], axis=-1).copy()
        sa_rgb = np.stack([sa_u8, sa_u8, sa_u8], axis=-1).copy()

        # Draw markers
        if self._predicted and self._ground_truth:
            self._draw_markers_on_view(ax_rgb, 'axial')
            self._draw_markers_on_view(co_rgb, 'coronal')
            self._draw_markers_on_view(sa_rgb, 'sagittal')

        # Convert to pixmaps
        self._pm_ax = self._array_to_pixmap(ax_rgb)
        self._pm_co = self._array_to_pixmap(co_rgb)
        self._pm_sa = self._array_to_pixmap(sa_rgb)

        self._rescale_all()

        # Update position text
        s_mm, a_mm, r_mm = self._spacing_SAR
        mm_ax = (self._iS - self._cS) * s_mm
        mm_co = (self._iA - self._cA) * a_mm
        mm_sa = (self._iR - self._cR) * r_mm
        self.lbl_co.set_br_text(f"{mm_co:.1f} mm")
        self.lbl_ax.set_br_text(f"{mm_ax:.1f} mm")
        self.lbl_sa.set_br_text(f"{mm_sa:.1f} mm")

    def _draw_markers_on_view(self, rgb: np.ndarray, view: str):
        """Draw predicted and ground truth markers."""
        h, w = rgb.shape[:2]
        S, A, R = self._vol_SAR.shape
        s_mm, a_mm, r_mm = self._spacing_SAR

        def mm_to_pixel(pos: MarkerPosition) -> Tuple[int, int]:
            """Convert physical coordinates to pixel in flipped slice."""
            # Physical position (mm) relative to volume center
            # Volume center is at (0, 0, 0) in physical space

            # Convert mm to array indices
            idx_S = self._cS + pos.z / s_mm
            idx_A = self._cA - pos.y / a_mm  # Anterior is negative A index direction
            idx_R = self._cR + pos.x / r_mm

            # Apply flipud/fliplr transform to get pixel coords
            if view == 'axial':
                # Slice[iS, :, :] with shape (A, R)
                # After fliplr: col = (R-1) - idx_R
                # After flipud: row = (A-1) - idx_A
                px = int((R - 1) - idx_R)
                py = int((A - 1) - idx_A)
            elif view == 'coronal':
                # Slice[:, iA, :] with shape (S, R)
                # After fliplr: col = (R-1) - idx_R
                # After flipud: row = (S-1) - idx_S
                px = int((R - 1) - idx_R)
                py = int((S - 1) - idx_S)
            else:  # sagittal
                # Slice[:, :, iR] with shape (S, A)
                # After fliplr: col = (A-1) - idx_A
                # After flipud: row = (S-1) - idx_S
                px = int((A - 1) - idx_A)
                py = int((S - 1) - idx_S)

            return max(8, min(w - 8, px)), max(8, min(h - 8, py))

        gt_x, gt_y = mm_to_pixel(self._ground_truth)
        pr_x, pr_y = mm_to_pixel(self._predicted)

        # Draw ground truth (green circle)
        self._draw_circle(rgb, gt_x, gt_y, 7, (0, 255, 0))
        # Draw prediction (red cross)
        self._draw_cross(rgb, pr_x, pr_y, 7, (255, 100, 100))
        # Draw error line (yellow)
        self._draw_line(rgb, gt_x, gt_y, pr_x, pr_y, (255, 255, 0))

    def _draw_circle(self, img, cx, cy, r, color):
        h, w = img.shape[:2]
        for angle in np.linspace(0, 2 * np.pi, 48):
            for dr in range(-1, 2):
                x = int(cx + (r + dr * 0.5) * np.cos(angle))
                y = int(cy + (r + dr * 0.5) * np.sin(angle))
                if 0 <= x < w and 0 <= y < h:
                    img[y, x] = color

    def _draw_cross(self, img, cx, cy, size, color):
        h, w = img.shape[:2]
        for d in range(-size, size + 1):
            for t in range(-1, 2):
                if 0 <= cx + d < w and 0 <= cy + t < h:
                    img[cy + t, cx + d] = color
                if 0 <= cx + t < w and 0 <= cy + d < h:
                    img[cy + d, cx + t] = color

    def _draw_line(self, img, x1, y1, x2, y2, color):
        h, w = img.shape[:2]
        steps = max(abs(x2 - x1), abs(y2 - y1), 1)
        for i in range(steps + 1):
            t = i / steps
            x, y = int(x1 + t * (x2 - x1)), int(y1 + t * (y2 - y1))
            if 0 <= x < w and 0 <= y < h:
                img[y, x] = color

    def _array_to_pixmap(self, rgb: np.ndarray) -> QPixmap:
        h, w, _ = rgb.shape
        qimg = QImage(np.ascontiguousarray(rgb).data, w, h, 3 * w, QImage.Format.Format_RGB888)
        return QPixmap.fromImage(qimg.copy())

    def _scales_from_spacing(self):
        """Calculate aspect ratio scales (matches CTQuadPanel)."""
        s_mm, a_mm, r_mm = self._spacing_SAR

        def norm(fx, fy):
            m = max(fx, fy, 1e-6)
            return (fx / m, fy / m)

        # Axial: cols=R, rows=A
        ax_scale = norm(r_mm, a_mm)
        # Coronal: cols=R, rows=S
        co_scale = norm(r_mm, s_mm)
        # Sagittal: cols=A, rows=S
        sa_scale = norm(a_mm, s_mm)

        return ax_scale, co_scale, sa_scale

    def _set_scaled_fit(self, lbl: QLabel, pm: Optional[QPixmap], fx: float, fy: float):
        """Set pixmap with aspect ratio correction (matches CTQuadPanel)."""
        if not pm or pm.isNull(): return
        lw, lh = lbl.width(), lbl.height()
        if lw <= 0 or lh <= 0: return
        pw, ph = pm.width(), pm.height()
        if pw <= 0 or ph <= 0: return

        scale = min(lw / (pw * fx), lh / (ph * fy))
        tw = max(1, int(pw * fx * scale))
        th = max(1, int(ph * fy * scale))

        lbl.setPixmap(pm.scaled(tw, th, Qt.AspectRatioMode.IgnoreAspectRatio,
                               Qt.TransformationMode.SmoothTransformation))

    def _rescale_all(self):
        """Rescale all views with correct aspect ratios."""
        ax_sc, co_sc, sa_sc = self._scales_from_spacing()

        if self._pm_co and not self._pm_co.isNull():
            self._set_scaled_fit(self.lbl_co, self._pm_co, *co_sc)
        if self._pm_ax and not self._pm_ax.isNull():
            self._set_scaled_fit(self.lbl_ax, self._pm_ax, *ax_sc)
        if self._pm_sa and not self._pm_sa.isNull():
            self._set_scaled_fit(self.lbl_sa, self._pm_sa, *sa_sc)

    def resizeEvent(self, e):
        super().resizeEvent(e)
        self._rescale_all()


# =============================================================================
# STATISTICS PANEL
# =============================================================================

class StatsInfoPanel(QGroupBox):
    """Statistics summary panel (matches info_panels.py style)."""

    def __init__(self, title: str = "Validation Statistics"):
        super().__init__(title)
        layout = QFormLayout(self)
        layout.setVerticalSpacing(6)

        self.lbl_tre = QLabel("—")
        self.lbl_tre.setStyleSheet("font-weight: bold; font-size: 13px;")
        self.lbl_p95 = QLabel("—")

        self.lbl_lr = QLabel("—")
        self.lbl_ap = QLabel("—")
        self.lbl_si = QLabel("—")

        layout.addRow("Mean TRE:", self.lbl_tre)
        layout.addRow("95th %ile:", self.lbl_p95)
        layout.addRow(QLabel(""))
        layout.addRow(QLabel("<b>Mean Error:</b>"))
        layout.addRow("  Left-Right:", self.lbl_lr)
        layout.addRow("  Ant-Post:", self.lbl_ap)
        layout.addRow("  Sup-Inf:", self.lbl_si)

    def update_stats(self, stats: ValidationStatistics):
        o = stats.overall
        d = stats.per_direction
        self.lbl_tre.setText(f"{o['mean']:.2f} ± {o['std']:.2f} mm")
        self.lbl_p95.setText(f"{o['p95']:.2f} mm")
        self.lbl_lr.setText(f"{d['LR']['mean']:.2f} mm")
        self.lbl_ap.setText(f"{d['AP']['mean']:.2f} mm")
        self.lbl_si.setText(f"{d['SI']['mean']:.2f} mm")


# =============================================================================
# RESULTS TABLE
# =============================================================================

class ResultsTable(QGroupBox):
    """Filterable results table."""
    result_selected = pyqtSignal(int)

    def __init__(self, title: str = "Results"):
        super().__init__(title)
        layout = QVBoxLayout(self)

        # Filters
        filter_row = QHBoxLayout()
        filter_row.addWidget(QLabel("Marker:"))
        self.cmb_marker = QComboBox()
        self.cmb_marker.addItem("All")
        self.cmb_marker.currentTextChanged.connect(self._filter)
        filter_row.addWidget(self.cmb_marker)
        filter_row.addStretch()
        layout.addLayout(filter_row)

        # Table
        self.table = QTableWidget()
        self.table.setColumnCount(5)
        self.table.setHorizontalHeaderLabels(["Marker", "Phase", "TRE", "Error (LR/AP/SI)", "Status"])
        self.table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.table.setSelectionMode(QTableWidget.SelectionMode.SingleSelection)
        self.table.itemSelectionChanged.connect(self._on_select)
        layout.addWidget(self.table)

        self._results: List[ValidationResult] = []
        self._filtered: List[ValidationResult] = []

    def update_data(self, results: List[ValidationResult]):
        self._results = results
        markers = sorted(set(f"Fiducial {r.marker_id}" for r in results))

        self.cmb_marker.blockSignals(True)
        self.cmb_marker.clear()
        self.cmb_marker.addItem("All")
        self.cmb_marker.addItems(markers)
        self.cmb_marker.blockSignals(False)
        self._filter()

    def _filter(self):
        marker_f = self.cmb_marker.currentText()
        filtered = self._results
        if marker_f != "All":
            mid = int(marker_f.split()[-1])
            filtered = [r for r in filtered if r.marker_id == mid]

        self._filtered = filtered
        self.table.setRowCount(len(filtered))
        for row, r in enumerate(filtered):
            self.table.setItem(row, 0, QTableWidgetItem(f"Fid {r.marker_id}"))
            self.table.setItem(row, 1, QTableWidgetItem(r.phase))

            t_item = QTableWidgetItem(f"{r.tre:.2f} mm")
            if r.tre < 2.0:
                t_item.setBackground(QColor(100, 200, 100))
            elif r.tre < 3.0:
                t_item.setBackground(QColor(255, 200, 100))
            else:
                t_item.setBackground(QColor(255, 100, 100))
            self.table.setItem(row, 2, t_item)

            self.table.setItem(row, 3, QTableWidgetItem(f"{r.error_lr:+.1f} / {r.error_ap:+.1f} / {r.error_si:+.1f}"))
            self.table.setItem(row, 4, QTableWidgetItem("Pass" if r.tre < 2 else "Check"))

    def _on_select(self):
        rows = self.table.selectedIndexes()
        if rows and self._filtered:
            original_idx = self._results.index(self._filtered[rows[0].row()])
            self.result_selected.emit(original_idx)


# =============================================================================
# MAIN PANEL
# =============================================================================

class ValidationPanel(QWidget):
    """Main DVF Validation Panel."""

    def __init__(self, parent=None):
        super().__init__(parent)

        # Main horizontal layout
        main = QHBoxLayout(self)
        main.setContentsMargins(4, 4, 4, 4)

        # Left: CT Viewer (60%)
        self.viewer = MarkerCTViewer()
        main.addWidget(self.viewer, 6)

        # Right: Stats & Table (40%)
        right = QWidget()
        right_lay = QVBoxLayout(right)
        right_lay.setContentsMargins(0, 0, 0, 0)

        self.stats = StatsInfoPanel()
        self.table = ResultsTable()
        self.table.result_selected.connect(self._on_result_selected)

        right_lay.addWidget(self.stats)
        right_lay.addWidget(self.table)

        main.addWidget(right, 4)

        # Load demo data
        self._results: List[ValidationResult] = []
        self._load_demo_data()

    def _load_demo_data(self):
        gen = DemoDataGenerator()
        self._results = gen.generate_validation_results()
        self.stats.update_stats(ValidationStatistics(self._results))
        self.table.update_data(self._results)
        if self._results:
            self.viewer.set_result(self._results[0])

    def _on_result_selected(self, idx: int):
        if 0 <= idx < len(self._results):
            self.viewer.set_result(self._results[idx])


# =============================================================================
# STANDALONE TEST
# =============================================================================

if __name__ == "__main__":
    import sys
    from PyQt6.QtWidgets import QApplication

    app = QApplication(sys.argv)
    panel = ValidationPanel()
    panel.setWindowTitle("VoxelMap DVF Validation")
    panel.resize(1100, 700)
    panel.show()
    sys.exit(app.exec())