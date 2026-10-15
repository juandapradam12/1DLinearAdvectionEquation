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
u_0(x) =
\begin{cases}
-x^{2} + \tfrac{1}{4}, & |x| \le \tfrac{1}{2}, \\
0, & \text{otherwise}.
\end{cases}
$$

Boundary values at the endpoints are held at their previous time level. For the short integration window used here, the pulse remains inside the domain, so boundary artifacts do not dominate the comparison.

## Spatial and temporal discretization

The domain is discretized with $N$ uniformly spaced nodes

$$
x_i = x_{\min} + i\,\Delta x, \qquad
\Delta x = \frac{x_{\max} - x_{\min}}{N - 1}, \qquad
i = 0,\ldots,N-1.
$$

Time is advanced with a constant step $\Delta t$. Let $u_i^{n} \approx u(x_i, n\Delta t)$. Because $c > 0$, information propagates from left to right, and a stable first-order scheme uses a backward (upwind) difference in space:

$$
u_i^{n+1}
=
u_i^{n}
-
\nu \left( u_i^{n} - u_{i-1}^{n} \right),
\qquad
\nu = \frac{c\,\Delta t}{\Delta x}.
$$

Here $\nu$ is the Courant–Friedrichs–Lewy (CFL) number. For this explicit upwind method, stability requires

$$
0 < \nu \le 1.
$$

## Truncation error and numerical diffusion

A Taylor expansion shows that the scheme is first-order accurate in both space and time. The leading error behaves like a dissipative term proportional to

$$
\frac{c\,\Delta x}{2}\,(1 - \nu)\,\frac{\partial^{2} u}{\partial x^{2}},
$$

so the discrete pulse broadens as it travels unless $\nu = 1$. In this repository the default parameters give $\nu < 1$, which makes the numerical diffusion visible when the solution is compared with the exact translate $u_0(x - c t)$.

## Error metric

At each requested snapshot time $t$, the relative $\ell^{2}$ discrepancy between the numerical field $u^{h}$ and the exact field $u$ is

$$
\varepsilon_{2}(t)
=
\frac{\lVert u^{h}(\cdot,t) - u(\cdot,t) \rVert_{2}}
     {\lVert u(\cdot,t) \rVert_{2}}.
$$

This scalar summary, together with the profile plots, is the quantitative evidence reported for the case study.
