# attach_run_dialog.py — pick run workspace + optional log for Continue run
from __future__ import annotations

from pathlib import Path

from PyQt6.QtWidgets import (
    QCheckBox,
    QDialog,
    QDialogButtonBox,
    QFileDialog,
    QFormLayout,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QVBoxLayout,
    QWidget,
)


class AttachRunDialog(QDialog):
    """
    Two-path attach: (1) run workspace where Patient…/train and outputs live,
    (2) optional custom log file (append). If log is automatic, uses Runs/logs/<run_name>/.
    """

    def __init__(self, parent=None, work_root: Path | None = None):
        super().__init__(parent)
        self._work_root = Path(work_root) if work_root else Path.home()
        self.setWindowTitle("Continue run — workspace & log")
        self.setMinimumWidth(520)

        root = QVBoxLayout(self)

        intro = QLabel(
            "Choose the run workspace (outputs go under Patient…/train for DICOM→MHA, DRR, DVF, etc.). "
            "Missing Patient…/train folders are created from your prescription path. "
            "Optional history log: paste a prior run transcript so the GUI can reopen the same CT/DRR/DVF tabs. "
            "Custom log below is only for where new lines append."
        )
        intro.setWordWrap(True)
        root.addWidget(intro)

        g = QGroupBox("Paths")
        fl = QFormLayout()

        run_row = QHBoxLayout()
        self._run_edit = QLineEdit()
        self._run_edit.setPlaceholderText("e.g. …/Runs/VALKIM_01_Run_20260329_141119")
        btn_run = QPushButton("Browse…")
        btn_run.clicked.connect(self._browse_run)
        run_row.addWidget(self._run_edit, 1)
        run_row.addWidget(btn_run)
        rw = QWidget()
        rw.setLayout(run_row)
        fl.addRow("Run folder:", rw)

        self._chk_custom_log = QCheckBox("Use a custom log file (append)")
        self._chk_custom_log.toggled.connect(self._toggle_log)
        fl.addRow(self._chk_custom_log)

        log_row = QHBoxLayout()
        self._log_edit = QLineEdit()
        self._log_edit.setEnabled(False)
        self._log_edit.setPlaceholderText("Pick a .log path — created if missing")
        btn_log = QPushButton("Browse…")
        btn_log.setObjectName("btn_log_browse")
        btn_log.clicked.connect(self._browse_log)
        log_row.addWidget(self._log_edit, 1)
        log_row.addWidget(btn_log)
        lw = QWidget()
        lw.setLayout(log_row)
        fl.addRow("Log file:", lw)

        hist_row = QHBoxLayout()
        self._hist_log_edit = QLineEdit()
        self._hist_log_edit.setPlaceholderText(
            "Optional: saved run .log — restores CT / DRR / DVF tabs from Step complete: lines"
        )
        btn_hist = QPushButton("Browse…")
        btn_hist.clicked.connect(self._browse_hist_log)
        hist_row.addWidget(self._hist_log_edit, 1)
        hist_row.addWidget(btn_hist)
        hw = QWidget()
        hw.setLayout(hist_row)
        fl.addRow("History log (viewers):", hw)

        g.setLayout(fl)
        root.addWidget(g)

        pipe = QGroupBox("Pipeline after attach")
        pv = QVBoxLayout()
        self._chk_launch_pipeline = QCheckBox("Run selected steps after attaching (otherwise attach only)")
        self._chk_launch_pipeline.setChecked(True)
        pv.addWidget(self._chk_launch_pipeline)
        self._chk_purge_downstream = QCheckBox(
            "Purge train/ outputs from the first selected step through the end of the pipeline"
        )
        self._chk_purge_downstream.setChecked(True)
        self._chk_purge_downstream.setToolTip(
            "Removes stale products for those steps so re-runs do not mix old and new files. "
            "Earlier steps (e.g. Copy) are left intact if you do not select them."
        )
        pv.addWidget(self._chk_purge_downstream)
        hint = QLabel(
            "Choose any combination of steps (e.g. start at DRR to avoid re-copying). "
            "Prerequisites are not enforced here—you are responsible for inputs already being in train/."
        )
        hint.setWordWrap(True)
        hint.setStyleSheet("color: #666;")
        pv.addWidget(hint)
        grid = QGridLayout()
        self._step_checks: list[QCheckBox] = []
        for i in range(9):
            c = QCheckBox()
            c.setChecked(True)
            self._step_checks.append(c)
            grid.addWidget(c, i, 0)
            lbl = QLabel("")
            lbl.setWordWrap(True)
            grid.addWidget(lbl, i, 1)
        pv.addLayout(grid)
        pipe.setLayout(pv)
        root.addWidget(pipe)
        self._pipe_group = pipe
        self._step_labels = [grid.itemAtPosition(i, 1).widget() for i in range(9)]

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self._try_accept)
        buttons.rejected.connect(self.reject)
        root.addWidget(buttons)

        self._sync_step_labels_and_checks_from_parent()

    def _sync_step_labels_and_checks_from_parent(self) -> None:
        p = self.parent()
        labels: list[str] = []
        if p is not None and hasattr(p, "tab2d"):
            td = p.tab2d
            if hasattr(td, "step_texts") and len(td.step_texts) >= len(self._step_checks):
                labels = list(td.step_texts)
            for i, chk in enumerate(self._step_checks):
                if i < len(td.step_checkboxes):
                    chk.setChecked(td.step_checkboxes[i].isChecked())
        if not labels:
            labels = [
                "1. Copy training files",
                "2. DICOM → MHA",
                "3. Downsample volumes",
                "4. Generate DRRs",
                "5. Compress DRRs",
                "6. 2D DVFs",
                "7. 3D DVFs (Downsampled)",
                "8. 3D DVFs (Full Resolution)",
                "9. kV TIFF → BIN (per fraction)",
            ]
        for i, w in enumerate(self._step_labels):
            if isinstance(w, QLabel) and i < len(labels):
                w.setText(labels[i])

    def launch_pipeline_after_attach(self) -> bool:
        return self._chk_launch_pipeline.isChecked()

    def purge_downstream_outputs(self) -> bool:
        return self._chk_purge_downstream.isChecked()

    def pipeline_step_flags(self) -> list[bool]:
        return [c.isChecked() for c in self._step_checks]

    def _toggle_log(self, on: bool) -> None:
        self._log_edit.setEnabled(on)
        for w in self.findChildren(QPushButton):
            if w.objectName() == "btn_log_browse":
                w.setEnabled(on)
                break

    def _browse_run(self) -> None:
        start = self._run_edit.text().strip() or str(self._work_root)
        d = QFileDialog.getExistingDirectory(self, "Run workspace folder", start)
        if d:
            self._run_edit.setText(d)

    def _browse_log(self) -> None:
        start = self._log_edit.text().strip() or str(self._work_root / "logs")
        path, _ = QFileDialog.getSaveFileName(
            self,
            "Log file (appends if file already exists)",
            start,
            "Log files (*.log);;All files (*)",
        )
        if path:
            self._log_edit.setText(path)

    def _browse_hist_log(self) -> None:
        start = self._hist_log_edit.text().strip() or str(self._work_root)
        path, _ = QFileDialog.getOpenFileName(
            self,
            "Run log to parse for completed steps",
            start,
            "Log files (*.log);;All files (*)",
        )
        if path:
            self._hist_log_edit.setText(path)

    def _try_accept(self) -> None:
        r = self._run_edit.text().strip()
        if not r:
            QMessageBox.warning(self, "Run folder", "Please set the run folder.")
            return
        if self._chk_launch_pipeline.isChecked() and not any(c.isChecked() for c in self._step_checks):
            QMessageBox.warning(
                self,
                "Pipeline steps",
                "You enabled “Run selected steps” but no steps are checked. "
                "Either check at least one step or turn off that option.",
            )
            return
        if self._chk_custom_log.isChecked():
            lg = self._log_edit.text().strip()
            if not lg:
                QMessageBox.warning(self, "Log file", "Please set a log file path or disable custom log.")
                return
        self.accept()

    @property
    def run_folder(self) -> Path:
        return Path(self._run_edit.text().strip())

    @property
    def custom_log_file(self) -> Path | None:
        if not self._chk_custom_log.isChecked():
            return None
        p = self._log_edit.text().strip()
        return Path(p) if p else None

    @property
    def optional_history_log(self) -> Path | None:
        p = self._hist_log_edit.text().strip()
        return Path(p) if p else None
