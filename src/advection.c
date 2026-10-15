#include "advection.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double initial_condition(double x)
{
    if (fabs(x) <= 0.5) {
        return -x * x + 0.25;
    }
    return 0.0;
}

double exact_solution(double x, double t, double c)
{
    return initial_condition(x - c * t);
}

int advection_init(AdvectionState *state, const AdvectionConfig *cfg)
{
    if (state == NULL || cfg == NULL) {
        return -1;
    }
    if (cfg->n_points < 3 || cfg->x_max <= cfg->x_min || cfg->dt <= 0.0) {
        return -1;
    }

    memset(state, 0, sizeof(*state));
    state->n_points = cfg->n_points;
    state->dx = (cfg->x_max - cfg->x_min) / (double)(cfg->n_points - 1);
    state->cfl = cfg->c * cfg->dt / state->dx;

    state->x = calloc((size_t)cfg->n_points, sizeof(double));
    state->u = calloc((size_t)cfg->n_points, sizeof(double));
    state->u_next = calloc((size_t)cfg->n_points, sizeof(double));

    if (state->x == NULL || state->u == NULL || state->u_next == NULL) {
        advection_free(state);
        return -1;
    }

    for (int i = 0; i < cfg->n_points; ++i) {
        state->x[i] = cfg->x_min + (double)i * state->dx;
        state->u[i] = initial_condition(state->x[i]);
    }

    return 0;
}

void advection_free(AdvectionState *state)
{
    if (state == NULL) {
        return;
    }
    free(state->x);
    free(state->u);
    free(state->u_next);
    state->x = NULL;
    state->u = NULL;
    state->u_next = NULL;
    state->n_points = 0;
}

void advection_step(AdvectionState *state, double cfl)
{
    const int n = state->n_points;

    /* Dirichlet boundaries held at the previous values (near-zero inflow/outflow). */
    state->u_next[0] = state->u[0];
    state->u_next[n - 1] = state->u[n - 1];

    for (int i = 1; i < n - 1; ++i) {
        state->u_next[i] = state->u[i] - cfl * (state->u[i] - state->u[i - 1]);
    }

    double *tmp = state->u;
    state->u = state->u_next;
    state->u_next = tmp;
}

static int write_snapshot(FILE *fp,
                          const AdvectionState *state,
                          double t,
                          double c)
{
    if (fprintf(fp, "# time=%.8f\n", t) < 0) {
        return -1;
    }
    if (fprintf(fp, "x,u_exact,u_numeric\n") < 0) {
        return -1;
    }

    for (int i = 0; i < state->n_points; ++i) {
        const double ue = exact_solution(state->x[i], t, c);
        if (fprintf(fp, "%.10e,%.10e,%.10e\n", state->x[i], ue, state->u[i]) < 0) {
            return -1;
        }
    }

    if (fprintf(fp, "\n") < 0) {
        return -1;
    }
    return 0;
}

static int should_dump(double t, double target, double dt)
{
    return fabs(t - target) < 0.5 * dt;
}

int advection_run(const AdvectionConfig *cfg,
                  const double *snapshot_times,
                  size_t n_snapshots,
                  const char *path)
{
    AdvectionState state;
    if (advection_init(&state, cfg) != 0) {
        fprintf(stderr, "error: failed to initialize advection state\n");
        return -1;
    }

    if (state.cfl > 1.0 + 1e-12) {
        fprintf(stderr,
                "warning: CFL = %.6f > 1; first-order upwind may be unstable\n",
                state.cfl);
    }

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "error: cannot open output file '%s'\n", path);
        advection_free(&state);
        return -1;
    }

    fprintf(fp,
            "# 1D linear advection | c=%.6f dx=%.6e dt=%.6e CFL=%.6f\n",
            cfg->c, state.dx, cfg->dt, state.cfl);

    size_t next_snap = 0;
    double t = 0.0;

    if (n_snapshots > 0 && should_dump(t, snapshot_times[0], cfg->dt)) {
        if (write_snapshot(fp, &state, t, cfg->c) != 0) {
            fclose(fp);
            advection_free(&state);
            return -1;
        }
        next_snap = 1;
    }

    for (int step = 0; step < cfg->n_steps; ++step) {
        advection_step(&state, state.cfl);
        t = (step + 1) * cfg->dt;

        while (next_snap < n_snapshots &&
               should_dump(t, snapshot_times[next_snap], cfg->dt)) {
            if (write_snapshot(fp, &state, t, cfg->c) != 0) {
                fclose(fp);
                advection_free(&state);
                return -1;
            }
            ++next_snap;
        }
    }

    fclose(fp);
    advection_free(&state);

    if (next_snap < n_snapshots) {
        fprintf(stderr,
                "warning: only wrote %zu of %zu requested snapshots "
                "(increase n_steps or adjust snapshot times)\n",
                next_snap, n_snapshots);
    }

    return 0;
}
