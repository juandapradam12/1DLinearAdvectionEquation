#!/usr/bin/env python3
"""Compare upwind, FTCS, and Lax–Wendroff on the same advection problem."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from analysis.io_util import load_snapshots, relative_l2_error, run_solver


def max_abs_series(
    snapshots: list[tuple[float, np.ndarray, np.ndarray, np.ndarray]],
) -> tuple[np.ndarray, np.ndarray]:
    times = np.asarray([t for t, *_ in snapshots], dtype=float)
    amps = np.asarray(
        [float(np.nanmax(np.abs(u_num))) for _, _, _, u_num in snapshots],
        dtype=float,
    )
    # Keep the plot readable once FTCS has clearly diverged.
    amps = np.where(np.isfinite(amps), amps, np.nan)
    return times, amps


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n-points", type=int, default=400)
    parser.add_argument("--cfl", type=float, default=0.5)
    parser.add_argument(
        "--profile-time",
        type=float,
        default=0.5,
        help="time used for the stable-scheme profile panel",
    )
    parser.add_argument(
        "--ftcs-final-time",
        type=float,
        default=1.5,
        help="longer horizon so FTCS amplitude growth is unmistakable",
    )
    parser.add_argument("--c", type=float, default=1.0)
    parser.add_argument("--x-min", type=float, default=-2.0)
    parser.add_argument("--x-max", type=float, default=2.0)
    parser.add_argument(
        "--ic",
        default="parabola",
        help="compact parabola seeds high modes that expose FTCS instability",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/assets/scheme_comparison.png"),
    )
    args = parser.parse_args()

    dx = (args.x_max - args.x_min) / (args.n_points - 1)
    dt = args.cfl * dx / args.c
    # Sample amplitude often enough for a smooth curve, not every micro-step.
    amp_every = max(dt, 0.01)

    profile_data: dict[str, tuple[np.ndarray, np.ndarray, np.ndarray]] = {}
    amp_data: dict[str, tuple[np.ndarray, np.ndarray]] = {}

    for scheme in ("upwind", "lax-wendroff"):
        out = Path("data") / f"scheme_{scheme}.csv"
        run_solver(
            output=out,
            scheme=scheme,
            ic=args.ic,
            n_points=args.n_points,
            c=args.c,
            x_min=args.x_min,
            x_max=args.x_max,
            dt=dt,
            final_time=args.ftcs_final_time,
            snapshot_every=amp_every,
        )
        snaps = load_snapshots(out)
        amp_data[scheme] = max_abs_series(snaps)

        chosen = snaps[-1]
        for t, x, u_ex, u_num in snaps:
            if abs(t - args.profile_time) <= 0.5 * amp_every:
                chosen = (t, x, u_ex, u_num)
                break
        t, x, u_ex, u_num = chosen
        err = relative_l2_error(u_ex, u_num)
        print(f"{scheme:12s}  t={t:.3f}  ε₂={err:.4e}  max|u|={np.max(np.abs(u_num)):.3e}")
        profile_data[scheme] = (x, u_ex, u_num)

    ftcs_out = Path("data") / "scheme_ftcs.csv"
    run_solver(
        output=ftcs_out,
        scheme="ftcs",
        ic=args.ic,
        n_points=args.n_points,
        c=args.c,
        x_min=args.x_min,
        x_max=args.x_max,
        dt=dt,
        final_time=args.ftcs_final_time,
        snapshot_every=amp_every,
    )
    ftcs_snaps = load_snapshots(ftcs_out)
    amp_data["ftcs"] = max_abs_series(ftcs_snaps)
    t_f, _, u_ex_f, u_num_f = ftcs_snaps[-1]
    print(
        f"{'ftcs':12s}  t={t_f:.3f}  ε₂={relative_l2_error(u_ex_f, u_num_f):.4e}  "
        f"max|u|={np.nanmax(np.abs(u_num_f)):.3e}"
    )

    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.3), constrained_layout=True)

    ax = axes[0]
    x_ref, u_ex, _ = profile_data["upwind"]
    ax.plot(x_ref, u_ex, "k--", lw=1.8, label="exact")
    for scheme in ("upwind", "lax-wendroff"):
        x, _, u_num = profile_data[scheme]
        ax.plot(x, u_num, lw=2.0, label=scheme)
    ax.set_xlabel(r"$x$ [m]")
    ax.set_ylabel(r"$u(x,t)$")
    ax.set_title(rf"Stable schemes at $t={args.profile_time:g}\,\mathrm{{s}}$")
    ax.legend(frameon=False)
    ax.grid(True, alpha=0.25)

    ax = axes[1]
    for scheme in ("upwind", "lax-wendroff", "ftcs"):
        times, amps = amp_data[scheme]
        # Truncate FTCS once it is clearly unstable to keep the axis useful.
        if scheme == "ftcs":
            mask = np.isfinite(amps) & (amps < 1e6)
            times, amps = times[mask], amps[mask]
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
