#include "advection.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static int failures = 0;

static void expect_true(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        ++failures;
    } else {
        printf("ok: %s\n", msg);
    }
}

static void expect_near(double got, double expected, double tol, const char *msg)
{
    if (!(fabs(got - expected) <= tol)) {
        fprintf(stderr,
                "FAIL: %s (got %.6e, expected %.6e, tol %.6e)\n",
                msg,
                got,
                expected,
                tol);
        ++failures;
    } else {
        printf("ok: %s\n", msg);
    }
}

static void test_init_and_cfl(void)
{
    AdvectionConfig cfg = {
        .c = 1.0,
        .x_min = -2.0,
        .x_max = 2.0,
        .n_points = 1000,
        .dt = 1e-3,
        .n_steps = 1,
        .scheme = SCHEME_UPWIND,
        .ic = IC_PARABOLA,
    };

    AdvectionState state;
    expect_true(advection_init(&state, &cfg) == 0, "advection_init succeeds");
    expect_true(state.cfl > 0.0 && state.cfl <= 1.0, "default CFL is in (0, 1]");
    expect_near(advection_relative_l2_error(&state, 0.0, cfg.c),
                0.0,
                1e-14,
                "relative L2 error at t=0 is zero");
    advection_free(&state);
}

static void test_upwind_error_bound(void)
{
    AdvectionConfig cfg = {
        .c = 1.0,
        .x_min = -2.0,
        .x_max = 2.0,
        .n_points = 1000,
        .dt = 1e-3,
        .n_steps = 600,
        .scheme = SCHEME_UPWIND,
        .ic = IC_PARABOLA,
    };

    AdvectionState state;
    expect_true(advection_init(&state, &cfg) == 0, "upwind init succeeds");

    for (int step = 0; step < cfg.n_steps; ++step) {
        advection_step(&state);
    }

    const double t = cfg.n_steps * cfg.dt;
    const double err = advection_relative_l2_error(&state, t, cfg.c);
    expect_true(err > 0.0, "upwind error at t=0.6 is positive (numerical diffusion)");
    expect_true(err < 5e-2, "upwind error at t=0.6 stays below 5e-2");
    advection_free(&state);
}

static void test_scheme_names(void)
{
    AdvectionScheme scheme;
    expect_true(advection_scheme_from_name("upwind", &scheme) == 0 &&
                    scheme == SCHEME_UPWIND,
                "parse upwind");
    expect_true(advection_scheme_from_name("ftcs", &scheme) == 0 &&
                    scheme == SCHEME_FTCS,
                "parse ftcs");
    expect_true(advection_scheme_from_name("lax-wendroff", &scheme) == 0 &&
                    scheme == SCHEME_LAX_WENDROFF,
                "parse lax-wendroff");
    expect_true(advection_scheme_from_name("nope", &scheme) != 0,
                "reject unknown scheme");
}

int main(void)
{
    test_scheme_names();
    test_init_and_cfl();
    test_upwind_error_bound();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
