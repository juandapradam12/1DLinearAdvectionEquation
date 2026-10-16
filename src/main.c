#include "advection.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_SNAPSHOTS 4096

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [options]\n"
            "\n"
            "Options:\n"
            "  --c VALUE              advection speed (default: 1.0)\n"
            "  --x-min VALUE          left boundary (default: -2.0)\n"
            "  --x-max VALUE          right boundary (default: 2.0)\n"
            "  --n-points N           spatial nodes (default: 1000)\n"
            "  --dt VALUE             time step (default: 1e-3)\n"
            "  --n-steps N            number of steps (default: 700)\n"
            "  --final-time T         sets n_steps = round(T / dt)\n"
            "  --scheme NAME          upwind | ftcs | lax-wendroff (default: upwind)\n"
            "  --ic NAME              parabola | gaussian (default: parabola)\n"
            "  --snapshots LIST       comma-separated times (default: 0,0.35,0.60)\n"
            "  --snapshot-every DT    generate snapshots every DT seconds from 0\n"
            "  --output PATH          CSV output path (default: data/advection_snapshots.csv)\n"
            "  -h, --help             show this help\n",
            prog);
}

static int make_data_dir(const char *path)
{
    char buf[512];
    const char *slash = strrchr(path, '/');
    if (slash == NULL) {
        return 0;
    }

    size_t len = (size_t)(slash - path);
    if (len == 0 || len >= sizeof(buf)) {
        return -1;
    }

    memcpy(buf, path, len);
    buf[len] = '\0';

    if (mkdir(buf, 0755) == 0 || errno == EEXIST) {
        return 0;
    }
    return -1;
}

static int parse_snapshots(const char *text, double *out, size_t *n_out)
{
    size_t n = 0;
    const char *p = text;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        char *end = NULL;
        const double value = strtod(p, &end);
        if (end == p) {
            return -1;
        }
        if (n >= MAX_SNAPSHOTS) {
            return -1;
        }
        out[n++] = value;

        p = end;
        while (*p == ' ' || *p == '\t') {
            ++p;
        }
        if (*p == ',') {
            ++p;
            continue;
        }
        if (*p != '\0') {
            return -1;
        }
    }

    *n_out = n;
    return n > 0 ? 0 : -1;
}

static int build_uniform_snapshots(double every,
                                   double final_time,
                                   double *out,
                                   size_t *n_out)
{
    if (every <= 0.0) {
        return -1;
    }

    size_t n = 0;
    for (double t = 0.0; t <= final_time + 0.5 * every; t += every) {
        if (n >= MAX_SNAPSHOTS) {
            return -1;
        }
        out[n++] = t;
    }
    *n_out = n;
    return n > 0 ? 0 : -1;
}

static int require_value(int argc, int i, const char *opt)
{
    if (i + 1 >= argc) {
        fprintf(stderr, "error: %s requires a value\n", opt);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *output = "data/advection_snapshots.csv";
    AdvectionConfig cfg = {
        .c = 1.0,
        .x_min = -2.0,
        .x_max = 2.0,
        .n_points = 1000,
        .dt = 1e-3,
        .n_steps = 700,
        .scheme = SCHEME_UPWIND,
        .ic = IC_PARABOLA,
    };

    double snapshots[MAX_SNAPSHOTS] = {0.0, 0.35, 0.60};
    size_t n_snapshots = 3;
    int have_final_time = 0;
    double final_time = 0.0;
    int have_snapshot_every = 0;
    double snapshot_every = 0.0;
    int have_snapshots_list = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--output") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            output = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--c") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            cfg.c = strtod(argv[++i], NULL);
            continue;
        }
        if (strcmp(argv[i], "--x-min") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            cfg.x_min = strtod(argv[++i], NULL);
            continue;
        }
        if (strcmp(argv[i], "--x-max") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            cfg.x_max = strtod(argv[++i], NULL);
            continue;
        }
        if (strcmp(argv[i], "--n-points") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            cfg.n_points = (int)strtol(argv[++i], NULL, 10);
            continue;
        }
        if (strcmp(argv[i], "--dt") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            cfg.dt = strtod(argv[++i], NULL);
            continue;
        }
        if (strcmp(argv[i], "--n-steps") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            cfg.n_steps = (int)strtol(argv[++i], NULL, 10);
            continue;
        }
        if (strcmp(argv[i], "--final-time") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            final_time = strtod(argv[++i], NULL);
            have_final_time = 1;
            continue;
        }
        if (strcmp(argv[i], "--scheme") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            if (advection_scheme_from_name(argv[++i], &cfg.scheme) != 0) {
                fprintf(stderr,
                        "error: unknown scheme '%s' "
                        "(use upwind, ftcs, or lax-wendroff)\n",
                        argv[i]);
                return 1;
            }
            continue;
        }
        if (strcmp(argv[i], "--ic") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            if (advection_ic_from_name(argv[++i], &cfg.ic) != 0) {
                fprintf(stderr,
                        "error: unknown initial condition '%s' "
                        "(use parabola or gaussian)\n",
                        argv[i]);
                return 1;
            }
            continue;
        }
        if (strcmp(argv[i], "--snapshots") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            if (parse_snapshots(argv[++i], snapshots, &n_snapshots) != 0) {
                fprintf(stderr, "error: invalid --snapshots list\n");
                return 1;
            }
            have_snapshots_list = 1;
            continue;
        }
        if (strcmp(argv[i], "--snapshot-every") == 0) {
            if (require_value(argc, i, argv[i]) != 0) {
                return 1;
            }
            snapshot_every = strtod(argv[++i], NULL);
            have_snapshot_every = 1;
            continue;
        }

        fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
        usage(argv[0]);
        return 1;
    }

    if (have_final_time) {
        if (final_time < 0.0 || cfg.dt <= 0.0) {
            fprintf(stderr, "error: invalid --final-time / --dt\n");
            return 1;
        }
        /* Ceil so the discrete horizon reaches the requested final time. */
        cfg.n_steps = (int)ceil(final_time / cfg.dt - 1e-12);
    } else {
        final_time = cfg.n_steps * cfg.dt;
    }

    if (have_snapshot_every) {
        if (have_snapshots_list) {
            fprintf(stderr,
                    "error: use either --snapshots or --snapshot-every, not both\n");
            return 1;
        }
        if (build_uniform_snapshots(snapshot_every, final_time, snapshots,
                                    &n_snapshots) != 0) {
            fprintf(stderr, "error: could not build snapshot schedule\n");
            return 1;
        }
    }

    if (make_data_dir(output) != 0) {
        fprintf(stderr, "error: could not create directory for '%s'\n", output);
        return 1;
    }

    const double dx = (cfg.x_max - cfg.x_min) / (double)(cfg.n_points - 1);
    const double cfl = cfg.c * cfg.dt / dx;

    printf("1D linear advection\n");
    printf("  scheme   = %s\n", advection_scheme_name(cfg.scheme));
    printf("  ic       = %s\n", advection_ic_name(cfg.ic));
    printf("  c        = %.6g m/s\n", cfg.c);
    printf("  domain   = [%.6g, %.6g] m\n", cfg.x_min, cfg.x_max);
    printf("  n_points = %d\n", cfg.n_points);
    printf("  dt       = %.6g s\n", cfg.dt);
    printf("  n_steps  = %d\n", cfg.n_steps);
    printf("  CFL      = %.6g\n", cfl);
    printf("  output   = %s\n", output);

    if (advection_run(&cfg, snapshots, n_snapshots, output) != 0) {
        return 1;
    }

    printf("done.\n");
    return 0;
}
