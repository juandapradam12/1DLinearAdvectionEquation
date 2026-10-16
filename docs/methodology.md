# Methodology

## Governing equation

This case study solves the one-dimensional linear advection equation

$$
\frac{\partial u}{\partial t} + c \frac{\partial u}{\partial x} = 0,
$$

where $u(x,t)$ is a transported scalar field and $c > 0$ is a constant advection speed. The equation states that the profile $u$ translates rigidly to the right at speed $c$, without changing shape in the continuum limit.

## Exact solution

For a constant speed $c$, the method of characteristics yields the traveling-wave solution

$$
u(x,t) = u_0(x - c t),
$$

with initial data $u_0(x) = u(x,0)$. This closed form is used as a reference to quantify numerical error.

## Initial and boundary data

The computational domain is $x \in [x_{\min}, x_{\max}] = [-2, 2]$. The initial pulse is the compact parabola

$$
u_0(x) = \begin{cases} -x^{2} + \tfrac{1}{4}, & |x| \le \tfrac{1}{2}, \\ 0, & \text{otherwise}. \end{cases}
$$

Boundary values at the endpoints are held at their previous time level. For the short integration window used here, the pulse remains inside the domain, so boundary artifacts do not dominate the comparison.

## Spatial grid and CFL number

The domain is discretized with $N$ uniformly spaced nodes

$$
x_i = x_{\min} + i\,\Delta x,\qquad \Delta x = \frac{x_{\max} - x_{\min}}{N - 1},\qquad i = 0,\ldots,N-1.
$$

Time is advanced with a constant step $\Delta t$. The Courant–Friedrichs–Lewy number is

$$
\nu = \frac{c\,\Delta t}{\Delta x}.
$$

## Finite-difference schemes

Let $u_i^{n} \approx u(x_i, n\Delta t)$. The repository implements three explicit updates.

### First-order upwind

Because $c > 0$, information propagates from left to right:

$$
u_i^{n+1} = u_i^{n} - \nu \left( u_i^{n} - u_{i-1}^{n} \right).
$$

Stability requires $0 < \nu \le 1$. The scheme is first-order accurate in space and time. Its leading error behaves like numerical diffusion proportional to

$$
\frac{c\,\Delta x}{2}\,(1 - \nu)\,\frac{\partial^{2} u}{\partial x^{2}}.
$$

### FTCS

Centered differences in space with forward Euler in time give

$$
u_i^{n+1} = u_i^{n} - \frac{\nu}{2}\left(u_{i+1}^{n} - u_{i-1}^{n}\right).
$$

For pure advection this method is unconditionally unstable: high-frequency modes grow for any $\nu > 0$. It is included as a negative control in the scheme comparison, integrated to a longer final time so the amplitude history makes the blow-up unmistakable.

### Lax–Wendroff

Adding the second-order Lax–Wendroff correction restores stability for $|\nu| \le 1$:

$$
u_i^{n+1} = u_i^{n} - \frac{\nu}{2}\left(u_{i+1}^{n} - u_{i-1}^{n}\right) + \frac{\nu^{2}}{2}\left(u_{i+1}^{n} - 2u_i^{n} + u_{i-1}^{n}\right).
$$

The method is second-order accurate and typically less dissipative than upwind, at the cost of mild dispersive ripples near steep gradients.

## Error metric

At each requested snapshot time $t$, the relative $\ell^{2}$ discrepancy between the numerical field $u^{h}$ and the exact field $u$ is

$$
\varepsilon_{2}(t) = \frac{\lVert u^{h}(\cdot,t) - u(\cdot,t) \rVert_{2}}{\lVert u(\cdot,t) \rVert_{2}}.
$$

## Convergence study

To expose the design order of upwind, the CFL number is held fixed while $N$ varies. Then $\Delta x$ and $\Delta t$ refine together, and a log–log fit of $\varepsilon_2$ versus $\Delta x$ should approach slope $1$.

The default profile figures use the compact parabola above. The convergence script switches to a smooth Gaussian

$$
u_0(x) = e^{-20 x^{2}}
$$

so that limited regularity at the parabola edges does not mask the first-order slope. The figure `docs/assets/convergence.png` reports the observed order for that smooth initial datum.
