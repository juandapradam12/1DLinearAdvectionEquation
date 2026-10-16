#!/usr/bin/env python3
"""Spatial convergence study for the first-order upwind scheme at fixed CFL."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from analysis.io_util import load_snapshots, relative_l2_error, run_solver


def observed_order(dx: np.ndarray, err: np.ndarray) -> float:
    """Least-squares slope of log(err) vs log(dx)."""
    coeff = np.polyfit(np.log(dx), np.log(err), 1)
    return float(coeff[0])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cfl", type=float, default=0.25)
    parser.add_argument("--final-time", type=float, default=0.5)
    parser.add_argument("--c", type=float, default=1.0)
    parser.add_argument("--x-min", type=float, default=-2.0)
    parser.add_argument("--x-max", type=float, default=2.0)
    parser.add_argument(
        "--n-points",
        type=int,
        nargs="+",
        default=[100, 200, 400, 800],
        help="grid sizes to compare",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/assets/convergence.png"),
    )
    args = parser.parse_args()

    dx_list: list[float] = []
    err_list: list[float] = []

    for n in args.n_points:
        dx = (args.x_max - args.x_min) / (n - 1)
        dt = args.cfl * dx / args.c
        out = Path("data") / f"convergence_n{n}.csv"
        run_solver(
            output=out,
            scheme="upwind",
            ic="gaussian",
            n_points=n,
            c=args.c,
            x_min=args.x_min,
            x_max=args.x_max,
            dt=dt,
            final_time=args.final_time,
            snapshots=str(args.final_time),
        )
        snaps = load_snapshots(out)
        _, _, u_ex, u_num = snaps[-1]
        err = relative_l2_error(u_ex, u_num)
        dx_list.append(dx)
        err_list.append(err)
        print(f"N={n:4d}  dx={dx:.4e}  dt={dt:.4e}  ε₂={err:.4e}")

    dx_arr = np.asarray(dx_list)
    err_arr = np.asarray(err_list)
    order = observed_order(dx_arr, err_arr)
    print(f"observed order ≈ {order:.3f} (expected ~ 1 for upwind)")

    fig, ax = plt.subplots(figsize=(6.2, 4.4), constrained_layout=True)
    ax.loglog(dx_arr, err_arr, "o-", lw=2.0, ms=7, label=r"upwind $\varepsilon_2$")

    ref = err_arr[0] * (dx_arr / dx_arr[0])
    ax.loglog(dx_arr, ref, "--", color="0.45", lw=1.6, label=r"slope $= 1$")

    ax.set_xlabel(r"$\Delta x$ [m]")
    ax.set_ylabel(r"$\varepsilon_2(t_{\mathrm{final}})$")
    ax.set_title(rf"Upwind convergence (Gaussian IC, CFL $= {args.cfl:g}$)")
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(frameon=False)
    ax.text(
        0.05,
        0.05,
        rf"observed order $\approx {order:.2f}$",
        transform=ax.transAxes,
        fontsize=10,
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=160)
    plt.close(fig)
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
