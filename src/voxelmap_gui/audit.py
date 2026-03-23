# audit.py
from __future__ import annotations
import time
from pathlib import Path
from typing import List, Optional


class LogFileDeletedError(Exception):
    """Raised when log file is deleted during run."""
    pass


class AuditLog:
    """Single text log; can be buffered in memory before a file is set."""
    def __init__(self, logfile: Optional[Path] = None):
        import os
        self._lines: List[str] = []
        self.logfile = Path(logfile) if logfile else None
        self._eol = os.linesep
        self._session_open = False
        self._run_open = False

    # --- session lifecycle ---
    def start_session(self, title: str = "Session") -> None:
        ts = time.strftime('%Y-%m-%d %H:%M:%S')
        header = f"=== {title} Started: {ts} ==="
        self._lines = [header, ""]
        self._session_open = True
        self._run_open = False
        if self.logfile:
            self.logfile.parent.mkdir(parents=True, exist_ok=True)
            with self.logfile.open("w", encoding="utf-8", newline="") as f:
                f.write(header + self._eol + self._eol)

    def set_log_file(self, path: Path, append: bool = False):
        """
        Sets the log file path and flushes buffered content if not appending.
        """
        self.logfile = path
        if self.logfile:
            self.logfile.parent.mkdir(parents=True, exist_ok=True)
            mode = "a" if append else "w"
            try:
                with self.logfile.open(mode, encoding="utf-8", newline="") as f:
                    if not append:
                        f.write(self._eol.join(self._lines) + self._eol)
            except Exception as e:
                print(f"ERROR: Failed to write audit log: {e}")

    def end_session(self, reason: str = "OK") -> None:
        if not self._session_open:
            return
        ts = time.strftime('%Y-%m-%d %H:%M:%S')
        tail = f"=== Session Ended ({reason}): {ts} ==="
        self.add_raw(f"\n{tail}")
        self._session_open = False

    def has_open_session(self) -> bool:
        return self._session_open

    # --- run (pipeline section) lifecycle ---
    def start_run(self) -> None:
        """Starts a new run section with a clear header."""
        ts = time.strftime('%Y-%m-%d %H:%M:%S')
        header = f"=== Pipeline Run Started: {ts} ==="
        # Add a blank line before the header for spacing
        self.add_raw(f"\n{header}")
        self._run_open = True

    def end_run(self, status: str = "OK") -> None:
        if not self._run_open:
            return
        ts = time.strftime('%Y-%m-%d %H:%M:%S')
        tail = f"=== Run Finished (Status: {status}) at {ts} ==="
        # Add blank lines around the footer for spacing
        self.add_raw(f"\n{tail}\n")
        self._run_open = False

    def has_open_run(self) -> bool:
        return self._run_open

    # --- atomic add ---
    def add(self, message: str, level: str = "INFO") -> str:
        """Adds a standard log entry with a short timestamp and level."""
        ts = time.strftime("%H:%M:%S")
        level_str = f"{level.upper():<7}" # Pad to 7 chars
        line = f"{ts} | {level_str} | {message}"
        self._lines.append(line)
        if self.logfile:
            try:
                with self.logfile.open("a", encoding="utf-8", newline="") as f:
                    f.write(line + self._eol)
            except FileNotFoundError:
                raise LogFileDeletedError(f"Log file deleted: {self.logfile}")
            except Exception:
                pass  # Other errors - continue
        return line

    def add_raw(self, message: str) -> None:
        """Adds a raw message to the log without any formatting."""
        self._lines.append(message)
        if self.logfile:
            try:
                with self.logfile.open("a", encoding="utf-8", newline="") as f:
                    f.write(message + self._eol)
            except FileNotFoundError:
                raise LogFileDeletedError(f"Log file deleted: {self.logfile}")
            except Exception:
                pass  # Other errors - continue

    def text(self) -> str:
        return "\n".join(self._lines)