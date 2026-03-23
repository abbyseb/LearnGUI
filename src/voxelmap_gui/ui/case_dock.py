# case_dock.py
from __future__ import annotations
from typing import List, Dict
from PyQt6.QtCore import pyqtSignal, Qt
from PyQt6.QtWidgets import (
    QDockWidget, QWidget, QVBoxLayout, QFormLayout,
    QComboBox, QLineEdit, QTableWidget, QTableWidgetItem,
    QLabel, QPushButton, QHBoxLayout, QAbstractItemView
)

from ..audit import AuditLog
from ..run_preparation import centre_from_rt  # Use authoritative version
from ..backend import BASE_API


class CaseDock(QDockWidget):
    selection_changed = pyqtSignal(list)

    def __init__(self, audit: AuditLog, parent=None):
        super().__init__("Case Selection", parent)
        self.audit = audit
        w = QWidget(); self.setWidget(w)
        v = QVBoxLayout(w)

        form = QFormLayout()
        self.cb_trial = QComboBox()
        self.cb_centre = QComboBox()
        self.txt_search = QLineEdit(); self.txt_search.setPlaceholderText("Search patient ID…")
        form.addRow("Trial", self.cb_trial)
        form.addRow("Centre", self.cb_centre)
        form.addRow("Search", self.txt_search)
        v.addLayout(form)

        self.cb_trial.addItem("Loading…"); self.cb_trial.setEnabled(False)
        self.cb_centre.addItem("All centres"); self.cb_centre.setEnabled(False)

        self.tbl = QTableWidget(0, 2)
        self.tbl.setHorizontalHeaderLabels(["Patient ID", "Centre"])
        self.tbl.horizontalHeader().setStretchLastSection(True)
        self.tbl.setSelectionBehavior(self.tbl.SelectionBehavior.SelectRows)
        self.tbl.setSelectionMode(self.tbl.SelectionMode.ExtendedSelection)
        self.tbl.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.tbl.setFocusPolicy(Qt.FocusPolicy.NoFocus)
        v.addWidget(self.tbl, 1)

        actions = QHBoxLayout()
        self.btn_select_all = QPushButton("Select all")
        self.btn_clear_sel = QPushButton("Clear selection")
        self.lbl_sel_count = QLabel("0 selected")
        actions.addWidget(self.btn_select_all); actions.addWidget(self.btn_clear_sel)
        actions.addStretch(1); actions.addWidget(self.lbl_sel_count)
        v.addLayout(actions)

        self.cb_trial.currentTextChanged.connect(self._on_trial_changed)
        self.cb_centre.currentTextChanged.connect(self._apply_filters)
        self.txt_search.textChanged.connect(self._apply_filters)
        self.tbl.itemSelectionChanged.connect(self._emit_selection)
        self.btn_select_all.clicked.connect(self.tbl.selectAll)
        self.btn_clear_sel.clicked.connect(self.tbl.clearSelection)

        self._all_patients: List[Dict[str, str]] = []
        self._load_trials()

    def _load_trials(self) -> None:
        trials = self._fetch_trials()
        self.cb_trial.blockSignals(True)
        self.cb_trial.clear()
        if trials:
            self.cb_trial.addItems(trials); self.cb_trial.setEnabled(True)
        else:
            self.cb_trial.addItem("—"); self.cb_trial.setEnabled(False)
        self.cb_trial.blockSignals(False)
        self._on_trial_changed(self.cb_trial.currentText())

    def _fetch_trials(self) -> List[str]:
        try:
            import requests
            r = requests.get(f"{BASE_API}/trials", timeout=20)
            r.raise_for_status()
            data = r.json()
            trials = [str(t) for t in data.get("trials", []) if t]
            trials = sorted(set(trials))
            if trials:
                return trials
        except Exception as e:
            self.audit.add(f"INFO: /trials unavailable, falling back: {e}")

        try:
            import requests
            r = requests.get(f"{BASE_API}/prescriptions", timeout=30)
            r.raise_for_status()
            pres = r.json().get("prescriptions", [])
            found = set()
            for p in pres:
                tr = p.get("trial")
                if tr:
                    found.add(str(tr)); continue
                rt = (p.get("rt_ct_pres") or "").strip("/")
                if rt:
                    segs = rt.split("/")
                    if segs:
                        found.add(segs[0])
            return sorted(found)
        except Exception as e:
            self.audit.add(f"ERROR: Failed to infer trials from prescriptions: {e}")
            return []

    def _load_centres_for_trial(self, trial: str) -> List[str]:
        try:
            import requests
            r = requests.get(f"{BASE_API}/centres", params={"trial": trial}, timeout=20)
            r.raise_for_status()
            data = r.json()
            centres = [str(c) for c in data.get("centres", []) if c]
            centres = sorted(set(centres))
            if centres:
                return centres
        except Exception:
            pass
        return []

    def _fetch_patients_for_trial(self, trial: str) -> List[Dict[str, str]]:
        try:
            import requests
            r = requests.get(f"{BASE_API}/prescriptions", params={"trial": trial}, timeout=30)
            r.raise_for_status()
            pres = r.json().get("prescriptions", [])
            rows: List[Dict[str, str]] = []
            for p in pres:
                pid = p.get("patient_trial_id", "") or p.get("patient_id", "")
                rt = (p.get("rt_ct_pres") or "").strip()
                centre = p.get("centre") or centre_from_rt(rt)
                if pid:
                    rows.append({"id": str(pid), "centre": str(centre) if centre else "?"})
            return rows
        except Exception as e:
            self.audit.add(f"ERROR: API list fetch failed: {e}")
            return []

    def _on_trial_changed(self, trial: str):
        rows = self._fetch_patients_for_trial(trial)
        self._all_patients = rows

        centres = self._load_centres_for_trial(trial)
        if not centres:
            centres = sorted({r["centre"] for r in rows if r.get("centre")})

        prev = self.cb_centre.currentText()
        self.cb_centre.blockSignals(True)
        self.cb_centre.clear()
        self.cb_centre.addItem("All centres")
        for c in centres:
            self.cb_centre.addItem(c)
        idx = self.cb_centre.findText(prev)
        self.cb_centre.setCurrentIndex(idx if idx >= 0 else 0)
        self.cb_centre.setEnabled(True)
        self.cb_centre.blockSignals(False)

        self._apply_filters()

    def _apply_filters(self):
        centre = self.cb_centre.currentText()
        term = self.txt_search.text().strip().lower()
        rows = self._all_patients
        if centre and centre != "All centres":
            rows = [r for r in rows if r.get("centre") == centre]
        if term:
            rows = [r for r in rows if term in (r.get("id", "").lower())]
        self._refresh_table(rows)

    def _refresh_table(self, rows: List[Dict[str, str]]):
        self.tbl.setRowCount(len(rows))
        for i, r in enumerate(rows):
            # Create items
            item_id = QTableWidgetItem(r.get("id", ""))
            item_centre = QTableWidgetItem(r.get("centre", ""))
            
            # Remove "Editable" flag explicitly so they are just text labels
            # We only keep Selectable and Enabled
            flags = Qt.ItemFlag.ItemIsSelectable | Qt.ItemFlag.ItemIsEnabled
            item_id.setFlags(flags)
            item_centre.setFlags(flags)

            self.tbl.setItem(i, 0, item_id)
            self.tbl.setItem(i, 1, item_centre)
            
        self.tbl.resizeColumnsToContents()
        self._emit_selection()

    def _emit_selection(self):
        sel = self.tbl.selectionModel().selectedRows() if self.tbl.selectionModel() else []
        ids = [self.tbl.item(i.row(), 0).text() for i in sel]
        self.lbl_sel_count.setText(f"{len(ids)} selected")
        self.selection_changed.emit(ids)

    def current_trial(self) -> str:
        return self.cb_trial.currentText()

    def current_centre(self) -> str:
        return self.cb_centre.currentText()