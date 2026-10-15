#!/usr/bin/env python3
"""Compare upwind, FTCS, and Lax–Wendroff on the same advection problem."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

from io_util import load_snapshots, relative_l2_error, run_solver  # noqa: E402


def max_abs_series(
    snapshots: list[tuple[float, np.ndarray, np.ndarray, np.ndarray]],
) -> tuple[np.ndarray, np.ndarray]:
    times = np.asarray([t for t, *_ in snapshots], dtype=float)
    amps = np.asarray([np.max(np.abs(u_num)) for _, _, _, u_num in snapshots], dtype=float)
    return times, amps


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n-points", type=int, default=400)
    parser.add_argument("--cfl", type=float, default=0.4)
    parser.add_argument("--final-time", type=float, default=0.5)
    parser.add_argument("--profile-time", type=float, default=0.5)
    parser.add_argument("--c", type=float, default=1.0)
    parser.add_argument("--x-min", type=float, default=-2.0)
    parser.add_argument("--x-max", type=float, default=2.0)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/assets/scheme_comparison.png"),
    )
    args = parser.parse_args()

    dx = (args.x_max - args.x_min) / (args.n_points - 1)
    dt = args.cfl * dx / args.c
    schemes = ("upwind", "lax-wendroff", "ftcs")

    profile_data: dict[str, tuple[np.ndarray, np.ndarray, np.ndarray]] = {}
    amp_data: dict[str, tuple[np.ndarray, np.ndarray]] = {}

    for scheme in schemes:
        out = Path("data") / f"scheme_{scheme}.csv"
        run_solver(
            output=out,
            scheme=scheme,
            n_points=args.n_points,
            c=args.c,
            x_min=args.x_min,
            x_max=args.x_max,
            dt=dt,
            final_time=args.final_time,
            snapshot_every=dt,
        )
        snaps = load_snapshots(out)
        amp_data[scheme] = max_abs_series(snaps)

        # Prefer the requested profile time; fall back to the last available frame.
        chosen = snaps[-1]
        for t, x, u_ex, u_num in snaps:
            if abs(t - args.profile_time) <= 0.5 * dt:
                chosen = (t, x, u_ex, u_num)
                break
        t, x, u_ex, u_num = chosen
        err = relative_l2_error(u_ex, u_num)
        print(f"{scheme:12s}  t={t:.3f}  ε₂={err:.4e}  max|u|={np.max(np.abs(u_num)):.3e}")
        profile_data[scheme] = (x, u_ex, u_num)

    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.3), constrained_layout=True)

    ax = axes[0]
    x_ref, u_ex, _ = profile_data["upwind"]
    ax.plot(x_ref, u_ex, "k--", lw=1.8, label="exact")
    for scheme, style in (
        ("upwind", "-"),
        ("lax-wendroff", "-"),
    ):
        x, _, u_num = profile_data[scheme]
        ax.plot(x, u_num, style, lw=2.0, label=scheme)
    ax.set_xlabel(r"$x$ [m]")
    ax.set_ylabel(r"$u(x,t)$")
    ax.set_title(rf"Stable schemes at $t={args.profile_time:g}\,\mathrm{{s}}$")
    ax.legend(frameon=False)
    ax.grid(True, alpha=0.25)

    ax = axes[1]
    for scheme in schemes:
        times, amps = amp_data[scheme]
        ax.semilogy(times, np.maximum(amps, 1e-16), lw=2.0, label=scheme)
    ax.set_xlabel(r"$t$ [s]")
    ax.set_ylabel(r"$\max |u|$")
    ax.set_title("Amplitude history (FTCS instability)")
    ax.legend(frameon=False)
    ax.grid(True, which="both", alpha=0.25)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=160)
    plt.close(fig)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
