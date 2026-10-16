#!/usr/bin/env python3
"""Post-process advection snapshots and produce comparison figures."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt

from analysis.io_util import load_snapshots, relative_l2_error


def plot_profiles(
    snapshots: list[tuple[float, object, object, object]],
    output: Path,
    scheme: str = "upwind",
) -> None:
    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.2), constrained_layout=True)

    ax = axes[0]
    for t, x, _, u_num in snapshots:
        ax.plot(x, u_num, lw=2.0, label=rf"$t = {t:.2f}\,\mathrm{{s}}$")
    ax.set_xlabel(r"$x$ [m]")
    ax.set_ylabel(r"$u(x,t)$")
    ax.set_title(f"Numerical solution ({scheme})")
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
    parser.add_argument("--scheme", default="upwind", help="label used in the title")
    args = parser.parse_args()

    snapshots = load_snapshots(args.input)
    print("snapshot errors (relative L2):")
    for t, _, u_ex, u_num in snapshots:
        err = relative_l2_error(u_ex, u_num)
        print(f"  t = {t:6.3f} s |  ε₂ = {err:.4e}")

    plot_profiles(snapshots, args.output, scheme=args.scheme)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
