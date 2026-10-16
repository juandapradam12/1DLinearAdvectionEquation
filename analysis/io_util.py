"""Shared helpers for reading solver CSV output and launching the C binary."""

from __future__ import annotations

import subprocess
from pathlib import Path

import numpy as np

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BIN = REPO_ROOT / "build" / "advection"


def load_snapshots(path: Path) -> list[tuple[float, np.ndarray, np.ndarray, np.ndarray]]:
    """Parse the multi-block CSV written by the C solver."""
    text = path.read_text()
    blocks: list[tuple[float, np.ndarray, np.ndarray, np.ndarray]] = []
    current_time: float | None = None
    rows: list[list[float]] = []

    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            if current_time is not None and rows:
                arr = np.asarray(rows, dtype=float)
                blocks.append((current_time, arr[:, 0], arr[:, 1], arr[:, 2]))
                rows = []
                current_time = None
            continue

        if line.startswith("# time="):
            current_time = float(line.split("=", 1)[1])
            continue
        if line.startswith("#") or line.startswith("x,"):
            continue

        parts = line.split(",")
        if len(parts) != 3:
            continue
        rows.append([float(p) for p in parts])

    if current_time is not None and rows:
        arr = np.asarray(rows, dtype=float)
        blocks.append((current_time, arr[:, 0], arr[:, 1], arr[:, 2]))

    if not blocks:
        raise ValueError(f"no snapshots found in {path}")
    return blocks


def relative_l2_error(u_exact: np.ndarray, u_numeric: np.ndarray) -> float:
    denom = np.linalg.norm(u_exact)
    if denom == 0.0:
        return float(np.linalg.norm(u_numeric))
    return float(np.linalg.norm(u_exact - u_numeric) / denom)


def run_solver(
    *,
    output: Path,
    scheme: str = "upwind",
    ic: str = "parabola",
    n_points: int = 1000,
    c: float = 1.0,
    x_min: float = -2.0,
    x_max: float = 2.0,
    dt: float | None = None,
    final_time: float = 0.6,
    snapshots: str | None = None,
    snapshot_every: float | None = None,
    bin_path: Path = DEFAULT_BIN,
) -> Path:
    """Invoke the compiled solver and return the output path."""
    if not bin_path.exists():
        raise FileNotFoundError(
            f"solver binary not found at {bin_path}; run `make` first"
        )

    output.parent.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(bin_path),
        "--scheme",
        scheme,
        "--ic",
        ic,
        "--n-points",
        str(n_points),
        "--c",
        str(c),
        "--x-min",
        str(x_min),
        "--x-max",
        str(x_max),
        "--final-time",
        str(final_time),
        "--output",
        str(output),
    ]
    if dt is not None:
        cmd.extend(["--dt", str(dt)])
    if snapshots is not None:
        cmd.extend(["--snapshots", snapshots])
    if snapshot_every is not None:
        cmd.extend(["--snapshot-every", str(snapshot_every)])

    subprocess.run(cmd, check=True, cwd=REPO_ROOT)
    return output
