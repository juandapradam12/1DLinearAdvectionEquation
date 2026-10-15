#!/usr/bin/env python3
"""Create a GIF of the advected pulse versus the exact traveling wave."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter

sys.path.insert(0, str(Path(__file__).resolve().parent))

from io_util import load_snapshots, run_solver  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n-points", type=int, default=600)
    parser.add_argument("--cfl", type=float, default=0.4)
    parser.add_argument("--final-time", type=float, default=0.6)
    parser.add_argument("--frame-every", type=float, default=0.02)
    parser.add_argument("--c", type=float, default=1.0)
    parser.add_argument("--x-min", type=float, default=-2.0)
    parser.add_argument("--x-max", type=float, default=2.0)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("docs/assets/advection_animation.gif"),
    )
    parser.add_argument("--fps", type=int, default=12)
    args = parser.parse_args()

    dx = (args.x_max - args.x_min) / (args.n_points - 1)
    dt = args.cfl * dx / args.c
    csv_path = Path("data") / "animation_frames.csv"

    run_solver(
        output=csv_path,
        scheme="upwind",
        n_points=args.n_points,
        c=args.c,
        x_min=args.x_min,
        x_max=args.x_max,
        dt=dt,
        final_time=args.final_time,
        snapshot_every=args.frame_every,
    )
    frames = load_snapshots(csv_path)

    fig, ax = plt.subplots(figsize=(7.2, 3.8), constrained_layout=True)
    x0, u_ex0, u_num0 = frames[0][1], frames[0][2], frames[0][3]
    (line_num,) = ax.plot(x0, u_num0, lw=2.2, label="numeric")
    (line_ex,) = ax.plot(x0, u_ex0, lw=1.8, ls="--", color="0.35", label="exact")
    title = ax.set_title(r"$t = 0.00\,\mathrm{s}$")
    ax.set_xlim(args.x_min, args.x_max)
    ax.set_ylim(-0.05, 0.30)
    ax.set_xlabel(r"$x$ [m]")
    ax.set_ylabel(r"$u(x,t)$")
    ax.legend(frameon=False, loc="upper right")
    ax.grid(True, alpha=0.25)

    def update(frame_index: int):
        t, x, u_ex, u_num = frames[frame_index]
        line_num.set_data(x, u_num)
        line_ex.set_data(x, u_ex)
        title.set_text(rf"$t = {t:.2f}\,\mathrm{{s}}$")
        return line_num, line_ex, title

    anim = FuncAnimation(fig, update, frames=len(frames), interval=1000 / args.fps)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    anim.save(args.output, writer=PillowWriter(fps=args.fps))
    plt.close(fig)
    print(f"wrote {args.output} ({len(frames)} frames)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
