#ifndef ADVECTION_H
#define ADVECTION_H

#include <stddef.h>

/** Supported explicit finite-difference schemes for 1D advection. */
typedef enum {
    SCHEME_UPWIND = 0,
    SCHEME_FTCS = 1,
    SCHEME_LAX_WENDROFF = 2
} AdvectionScheme;

/** Initial-condition shapes available to the solver. */
typedef enum {
    IC_PARABOLA = 0, /* compact support; limited smoothness */
    IC_GAUSSIAN = 1  /* smooth bump for clean convergence tests */
} InitialCondition;

/**
 * Parameters for the 1D linear advection problem
 *   ∂u/∂t + c ∂u/∂x = 0
 * on a uniform grid.
 */
typedef struct {
    double c;                 /* advection speed [m/s] */
    double x_min;             /* left domain boundary [m] */
    double x_max;             /* right domain boundary [m] */
    int n_points;             /* number of spatial nodes */
    double dt;                /* time step [s] */
    int n_steps;              /* number of time steps */
    AdvectionScheme scheme;   /* spatial/temporal discretization */
    InitialCondition ic;      /* initial profile */
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
    AdvectionScheme scheme;
    InitialCondition ic;
} AdvectionState;

/** Evaluate the selected initial condition at x. */
double initial_condition(double x, InitialCondition ic);

/** Exact traveling-wave solution u(x, t) = u0(x - c t). */
double exact_solution(double x, double t, double c, InitialCondition ic);

const char *advection_ic_name(InitialCondition ic);
int advection_ic_from_name(const char *name, InitialCondition *out);

const char *advection_scheme_name(AdvectionScheme scheme);
int advection_scheme_from_name(const char *name, AdvectionScheme *out);

int advection_init(AdvectionState *state, const AdvectionConfig *cfg);
void advection_free(AdvectionState *state);

/** Advance one time step with the scheme stored in `state`. */
void advection_step(AdvectionState *state);

/** Relative discrete ℓ² error against the exact solution at time t. */
double advection_relative_l2_error(const AdvectionState *state, double t, double c);

/**
 * Run the full simulation and write selected snapshots to `path` as CSV:
 *   x,u_exact,u_numeric
 * Multiple snapshot times are written sequentially with a blank line separator.
 */
int advection_run(const AdvectionConfig *cfg,
                  const double *snapshot_times,
                  size_t n_snapshots,
                  const char *path);

#endif /* ADVECTION_H */
