# 1D Linear Advection — Numerical Case Study

A compact scientific-computing portfolio piece: the one-dimensional linear advection equation is discretized with a first-order upwind finite-difference scheme in C, then compared against the exact traveling-wave solution and visualized in Python.

$$
\frac{\partial u}{\partial t} + c \frac{\partial u}{\partial x} = 0
$$

<p align="center">
  <img src="docs/assets/advection_profiles.png" alt="Advection profiles: numeric solution and exact comparison" width="820"/>
</p>

## Why this case study

Advection is a minimal hyperbolic PDE that still exposes the design choices that matter in production solvers: CFL stability, upwind bias, numerical diffusion, and verification against a known exact solution. The repository keeps the physics small so the software structure stays easy to inspect.

## Architecture

| Layer | Role |
| --- | --- |
| `include/advection.h`, `src/advection.c` | Problem API: configuration, initial/exact fields, upwind step, CSV export |
| `src/main.c` | Thin driver with the case-study defaults and CLI output path |
| `python/plot_results.py` | Snapshot parsing, relative $\ell^{2}$ errors, publication-style figures |
| `docs/methodology.md` | Governing equation, discretization, and error metric |

Build orchestration lives in the `Makefile`. Simulation data is written under `data/`; figures under `docs/assets/`.

## Default experiment

| Parameter | Value |
| --- | --- |
| Advection speed $c$ | $1.0\,\mathrm{m/s}$ |
| Domain | $x \in [-2, 2]$ |
| Grid points $N$ | $1000$ |
| Time step $\Delta t$ | $10^{-3}\,\mathrm{s}$ |
| Steps | $700$ |
| Snapshots | $t \in \{0,\,0.35,\,0.60\}\,\mathrm{s}$ |
| Initial data | compact parabola $u_0(x) = -x^{2} + 1/4$ for $\|x\| \le 1/2$ |

The continuum solution is the rigid translate $u(x,t) = u_0(x - c t)$. With CFL number $\nu = c\Delta t/\Delta x < 1$, the upwind scheme remains stable but introduces visible numerical diffusion — intentional here, so the numeric vs exact panel is informative.

## Quick start

```bash
# dependencies for plotting
python3 -m pip install -r requirements.txt

# build, run, and generate docs/assets/advection_profiles.png
make plot
```

Equivalent steps:

```bash
make
./build/advection
python3 python/plot_results.py
```

Optional output path:

```bash
./build/advection --output data/custom.csv
python3 python/plot_results.py --input data/custom.csv
```

## Notable improvements over the original exercise code

- Modular C API with explicit init / step / free lifecycle and `double` precision
- Correct grid spacing $\Delta x = (x_{\max}-x_{\min})/(N-1)$ and boundary updates
- CFL diagnostic and exact-solution verification in the exported CSV
- Structured post-processing with relative $\ell^{2}$ error reporting
- Removal of unrelated OpenMP hello-world and cluster job-script leftovers

## Documentation

- [Methodology](docs/methodology.md) — PDE, scheme, stability, and error metric with rendered LaTeX

## License

Personal portfolio code. Reuse with attribution is welcome unless a separate license file is added later.
