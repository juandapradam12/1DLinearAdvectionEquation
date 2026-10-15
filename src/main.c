#include "advection.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--output PATH]\n"
            "\n"
            "Runs the 1D linear advection case study with default parameters\n"
            "and writes CSV snapshots for post-processing.\n",
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

int main(int argc, char **argv)
{
    const char *output = "data/advection_snapshots.csv";

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
            continue;
        }
        fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
        usage(argv[0]);
        return 1;
    }

    /* Case-study defaults matching the original numerical experiment. */
    AdvectionConfig cfg = {
        .c = 1.0,
        .x_min = -2.0,
        .x_max = 2.0,
        .n_points = 1000,
        .dt = 1e-3,
        .n_steps = 700,
    };

    const double snapshots[] = {0.0, 0.35, 0.60};

    if (make_data_dir(output) != 0) {
        fprintf(stderr, "error: could not create directory for '%s'\n", output);
        return 1;
    }

    printf("1D linear advection\n");
    printf("  c        = %.3f m/s\n", cfg.c);
    printf("  domain   = [%.1f, %.1f] m\n", cfg.x_min, cfg.x_max);
    printf("  n_points = %d\n", cfg.n_points);
    printf("  dt       = %.3e s\n", cfg.dt);
    printf("  n_steps  = %d\n", cfg.n_steps);
    printf("  output   = %s\n", output);

    if (advection_run(&cfg, snapshots, sizeof(snapshots) / sizeof(snapshots[0]),
                      output) != 0) {
        return 1;
    }

    printf("done.\n");
    return 0;
}
