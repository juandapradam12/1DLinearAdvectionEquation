#ifndef ADVECTION_H
#define ADVECTION_H

#include <stddef.h>

/**
 * Parameters for the 1D linear advection problem
 *   ∂u/∂t + c ∂u/∂x = 0
 * on a uniform grid with first-order upwind time marching.
 */
typedef struct {
    double c;       /* advection speed [m/s] */
    double x_min;   /* left domain boundary [m] */
    double x_max;   /* right domain boundary [m] */
    int n_points;   /* number of spatial nodes */
    double dt;      /* time step [s] */
    int n_steps;    /* number of time steps */
} AdvectionConfig;

/**
 * Simulation state: grid coordinates and the current field u(x).
 * Ownership of the arrays belongs to the caller after init/free.
 */
typedef struct {
    double *x;
    double *u;
    double *u_next;
    int n_points;
    double dx;
    double cfl;
} AdvectionState;

/** Compact initial condition used in this case study. */
double initial_condition(double x);

/** Exact traveling-wave solution u(x, t) = u0(x - c t). */
double exact_solution(double x, double t, double c);

int advection_init(AdvectionState *state, const AdvectionConfig *cfg);
void advection_free(AdvectionState *state);

/**
 * Advance the solution by one time step using the first-order upwind scheme.
 * Requires CFL = c * dt / dx ≤ 1 for stability when c > 0.
 */
void advection_step(AdvectionState *state, double cfl);

/**
 * Run the full simulation and write selected snapshots to `path` as CSV:
 *   x,u_exact,u_numeric,time
 * Multiple snapshot times are written sequentially with a blank line separator.
 */
int advection_run(const AdvectionConfig *cfg,
                  const double *snapshot_times,
                  size_t n_snapshots,
                  const char *path);

#endif /* ADVECTION_H */
