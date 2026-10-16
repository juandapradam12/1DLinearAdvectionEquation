#include "advection.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double initial_condition(double x, InitialCondition ic)
{
    switch (ic) {
    case IC_GAUSSIAN:
        return exp(-20.0 * x * x);
    case IC_PARABOLA:
    default:
        if (fabs(x) <= 0.5) {
            return -x * x + 0.25;
        }
        return 0.0;
    }
}

double exact_solution(double x, double t, double c, InitialCondition ic)
{
    return initial_condition(x - c * t, ic);
}

const char *advection_ic_name(InitialCondition ic)
{
    switch (ic) {
    case IC_GAUSSIAN:
        return "gaussian";
    case IC_PARABOLA:
    default:
        return "parabola";
    }
}

int advection_ic_from_name(const char *name, InitialCondition *out)
{
    if (name == NULL || out == NULL) {
        return -1;
    }
    if (strcmp(name, "parabola") == 0) {
        *out = IC_PARABOLA;
        return 0;
    }
    if (strcmp(name, "gaussian") == 0) {
        *out = IC_GAUSSIAN;
        return 0;
    }
    return -1;
}

const char *advection_scheme_name(AdvectionScheme scheme)
{
    switch (scheme) {
    case SCHEME_UPWIND:
        return "upwind";
    case SCHEME_FTCS:
        return "ftcs";
    case SCHEME_LAX_WENDROFF:
        return "lax-wendroff";
    default:
        return "unknown";
    }
}

int advection_scheme_from_name(const char *name, AdvectionScheme *out)
{
    if (name == NULL || out == NULL) {
        return -1;
    }
    if (strcmp(name, "upwind") == 0) {
        *out = SCHEME_UPWIND;
        return 0;
    }
    if (strcmp(name, "ftcs") == 0) {
        *out = SCHEME_FTCS;
        return 0;
    }
    if (strcmp(name, "lax-wendroff") == 0 || strcmp(name, "lw") == 0) {
        *out = SCHEME_LAX_WENDROFF;
        return 0;
    }
    return -1;
}

int advection_init(AdvectionState *state, const AdvectionConfig *cfg)
{
    if (state == NULL || cfg == NULL) {
        return -1;
    }
    if (cfg->n_points < 3 || cfg->x_max <= cfg->x_min || cfg->dt <= 0.0 ||
        cfg->n_steps < 0) {
        return -1;
    }

    memset(state, 0, sizeof(*state));
    state->n_points = cfg->n_points;
    state->dx = (cfg->x_max - cfg->x_min) / (double)(cfg->n_points - 1);
    state->cfl = cfg->c * cfg->dt / state->dx;
    state->scheme = cfg->scheme;
    state->ic = cfg->ic;

    state->x = calloc((size_t)cfg->n_points, sizeof(double));
    state->u = calloc((size_t)cfg->n_points, sizeof(double));
    state->u_next = calloc((size_t)cfg->n_points, sizeof(double));

    if (state->x == NULL || state->u == NULL || state->u_next == NULL) {
        advection_free(state);
        return -1;
    }

    for (int i = 0; i < cfg->n_points; ++i) {
        state->x[i] = cfg->x_min + (double)i * state->dx;
        state->u[i] = initial_condition(state->x[i], state->ic);
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

void advection_step(AdvectionState *state)
{
    const int n = state->n_points;
    const double nu = state->cfl;

    state->u_next[0] = state->u[0];
    state->u_next[n - 1] = state->u[n - 1];

    switch (state->scheme) {
    case SCHEME_UPWIND:
        for (int i = 1; i < n - 1; ++i) {
            state->u_next[i] = state->u[i] - nu * (state->u[i] - state->u[i - 1]);
        }
        break;

    case SCHEME_FTCS:
        for (int i = 1; i < n - 1; ++i) {
            state->u_next[i] =
                state->u[i] - 0.5 * nu * (state->u[i + 1] - state->u[i - 1]);
        }
        break;

    case SCHEME_LAX_WENDROFF:
        for (int i = 1; i < n - 1; ++i) {
            const double du = state->u[i + 1] - state->u[i - 1];
            const double d2u = state->u[i + 1] - 2.0 * state->u[i] + state->u[i - 1];
            state->u_next[i] = state->u[i] - 0.5 * nu * du + 0.5 * nu * nu * d2u;
        }
        break;
    }

    double *tmp = state->u;
    state->u = state->u_next;
    state->u_next = tmp;
}

double advection_relative_l2_error(const AdvectionState *state, double t, double c)
{
    double num = 0.0;
    double den = 0.0;

    for (int i = 0; i < state->n_points; ++i) {
        const double ue = exact_solution(state->x[i], t, c, state->ic);
        const double diff = state->u[i] - ue;
        num += diff * diff;
        den += ue * ue;
    }

    if (den == 0.0) {
        return sqrt(num);
    }
    return sqrt(num / den);
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
        const double ue = exact_solution(state->x[i], t, c, state->ic);
        if (fprintf(fp, "%.10e,%.10e,%.10e\n", state->x[i], ue, state->u[i]) < 0) {
            return -1;
        }
    }

    if (fprintf(fp, "\n") < 0) {
        return -1;
    }
    return 0;
}

static void warn_cfl(const AdvectionState *state)
{
    const double nu = state->cfl;
    if (state->scheme == SCHEME_FTCS) {
        fprintf(stderr,
                "warning: FTCS is unconditionally unstable for pure advection "
                "(CFL = %.6f)\n",
                nu);
        return;
    }
    if (nu > 1.0 + 1e-12) {
        fprintf(stderr,
                "warning: CFL = %.6f > 1; %s may be unstable\n",
                nu,
                advection_scheme_name(state->scheme));
    }
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

    warn_cfl(&state);

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "error: cannot open output file '%s'\n", path);
        advection_free(&state);
        return -1;
    }

    fprintf(fp,
            "# 1D linear advection | scheme=%s ic=%s c=%.6f dx=%.6e dt=%.6e CFL=%.6f\n",
            advection_scheme_name(cfg->scheme),
            advection_ic_name(cfg->ic),
            cfg->c,
            state.dx,
            cfg->dt,
            state.cfl);

    size_t next_snap = 0;
    double t = 0.0;

    while (next_snap < n_snapshots &&
           t + 0.5 * cfg->dt >= snapshot_times[next_snap]) {
        if (write_snapshot(fp, &state, t, cfg->c) != 0) {
            fclose(fp);
            advection_free(&state);
            return -1;
        }
        ++next_snap;
    }

    for (int step = 0; step < cfg->n_steps; ++step) {
        advection_step(&state);
        t = (step + 1) * cfg->dt;

        while (next_snap < n_snapshots &&
               t + 0.5 * cfg->dt >= snapshot_times[next_snap]) {
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
                next_snap,
                n_snapshots);
    }

    return 0;
}
