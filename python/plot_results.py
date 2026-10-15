#!/usr/bin/env python3
"""Post-process advection snapshots and produce comparison figures."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


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


def plot_profiles(
    snapshots: list[tuple[float, np.ndarray, np.ndarray, np.ndarray]],
    output: Path,
) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.2), constrained_layout=True)

    ax = axes[0]
    for t, x, _, u_num in snapshots:
        ax.plot(x, u_num, lw=2.0, label=rf"$t = {t:.2f}\,\mathrm{{s}}$")
    ax.set_xlabel(r"$x$ [m]")
    ax.set_ylabel(r"$u(x,t)$")
    ax.set_title("Numerical solution (upwind)")
    ax.legend(frameon=False)
    ax.grid(True, alpha=0.25)

    ax = axes[1]
    for t, x, u_ex, u_num in snapshots:
        if t == 0.0:
            continue
        ax.plot(x, u_ex, lw=1.8, ls="--", color="0.35", label=rf"exact $t={t:.2f}$")
        ax.plot(x, u_num, lw=2.0, label=rf"numeric $t={t:.2f}$")
    ax.set_xlabel(r"$x$ [m]")
    ax.set_ylabel(r"$u(x,t)$")
    ax.set_title("Exact vs numeric")
    ax.legend(frameon=False, fontsize=8)
    ax.grid(True, alpha=0.25)

    output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(output, dpi=160)
    plt.close(fig)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input",
        type=Path,
        default=Path("data/advection_snapshots.csv"),
        help="CSV produced by the C solver",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/assets/advection_profiles.png"),
        help="figure path",
    )
    args = parser.parse_args()

    snapshots = load_snapshots(args.input)
    print("snapshot errors (relative L2):")
    for t, _, u_ex, u_num in snapshots:
        err = relative_l2_error(u_ex, u_num)
        print(f"  t = {t:6.3f} s |  ε₂ = {err:.4e}")

    plot_profiles(snapshots, args.output)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
