#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "config.h"
#include "keys.h"
#include "../lib/sarray/sarray.h"

void help(char *progname) {
    fprintf(stderr,
"Usage:\n"
"    %s config_file [--mode-logs] < log_file\n", progname);
}

void parse_args(int argc, char *argv[], char **conf_path, int *mode_logs) {
    if (argc < 2) {
        fprintf(stderr, "Too few arguments.\n");
        goto fail;
    }
    *conf_path = argv[1];
    *mode_logs = 0;
    if (argc > 2) {
        if (strcmp(argv[2], "--mode-logs") == 0) {
            *mode_logs = 1;
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argv[2]);
            goto fail;
        }
    }
    return;

fail:
    help(argv[0]);
    exit(EXIT_FAILURE);
}

int read_line(char *key, long *secs, long *msecs) {
    int getc_match = getchar();
    int scanf_match;
    if (getc_match == EOF) {
        return EOF;
    } else {
        *(unsigned char *)key = getc_match;
        getc_match = 1;
    }
    scanf_match = scanf(" %ld.%ld", secs, msecs);
    // Read the EOL
    getchar();
    if (scanf_match == EOF) {
        return EOF;
    } else {
        return getc_match + scanf_match;
    }
}

FILE *open_mode_log(config *conf, vmode_t mode) {
    char new_filename[30];
    int new_fn_len;
    new_fn_len = snprintf(new_filename,
                          sizeof(new_filename),
                          "mode-%s.log",
                          config_mode_name(conf, mode));
    if (new_fn_len > sizeof(new_filename)) {
        new_filename[sizeof(new_filename)-1] = '\0';
    }
    return fopen(new_filename, "a");
}

void produce_mode_logs(config *conf) {
    int matched;
    char key;
    long secs, msecs;
    vmode_t mode = config_init_mode(conf);
    vmode_t new_mode;
    FILE *mode_log = open_mode_log(conf, mode);

    if (mode_log == NULL) {
        fprintf(stderr, "Could not open log file for mode: %d\n", mode);
        goto fail;
    }

    printf("%s: ", config_mode_name(conf, mode));
    while ((matched = read_line(&key, &secs, &msecs)) != EOF) {
        if (matched != 3) {
            fprintf(stderr, "Scanf matched %d elements, expected 3\n", matched);
            goto fail;
        }

        {
            char *str;
            str = hreadable_from_key(key);
            printf("%s", str);
            fprintf(mode_log, "%s", str);
            free(str);
        }
        if (config_transition(conf, mode, key, &new_mode)) {
            printf("\n");
            mode = new_mode;
            printf("Changing mode to %s:\n", config_mode_name(conf, mode));

            fprintf(mode_log, "\n\n");
            fclose(mode_log);
            mode_log = open_mode_log(conf, new_mode);
            if (mode_log == NULL) {
                fprintf(stderr, "Could not open log file for mode: %d\n", mode);
                goto fail;
            }
        }
    }
    printf("\n");

    fprintf(mode_log, "\n\n");
    fclose(mode_log);
    return;

fail:
    config_free(conf);
    exit(EXIT_FAILURE);
}

#define INIT_ARR_LEN 1024
struct timedesc {
    long secs, msecs;
};

struct timedesc tstamps_add(struct timedesc a, struct timedesc b) {
    struct timedesc res;
    res = a;
    res.msecs += b.msecs;
    res.secs += res.msecs / (1000 * 1000);
    res.msecs %= (1000 * 1000);
    res.secs += b.secs;
    return res;
}

struct timedesc tstamps_sub(struct timedesc a, struct timedesc b) {
    struct timedesc res;
    res.secs = a.secs - b.secs;
    if (a.msecs < b.msecs) {
        res.secs -= 1;
        res.msecs = (1000 * 1000) + a.msecs - b.msecs;
    } else {
        res.msecs = a.msecs - b.msecs;
    }
    return res;
}

#define LONG_DELAY_SECS 3

int long_key_delay(struct timedesc a, struct timedesc b) {
    struct timedesc diff = tstamps_sub(b, a);
    return (diff.secs > LONG_DELAY_SECS);
}

int extend_arrs(size_t *arrlen, char **keys, struct timedesc **tstamps) {
    void *tmp;

    *arrlen *= 2;
    tmp = realloc(*keys, *arrlen * sizeof(**keys));
    if (tmp == NULL) {
        fprintf(stderr, "Realloc failed.\n");
        return 0;
    }
    *keys = tmp;
    tmp = realloc(*tstamps, *arrlen * sizeof(**tstamps));
    if (tmp == NULL) {
        fprintf(stderr, "Realloc failed.\n");
        return 0;
    }
    *tstamps = tmp;
    return 1;
}

void gen_key_time_data(config *conf, char **out_keys,
                       struct timedesc **out_tstamps,
                       size_t *len) {
    int matched;
    char key;
    vmode_t mode = config_init_mode(conf);
    vmode_t new_mode;

    struct timedesc key_time, prev_time;

    char *keys;
    struct timedesc *tstamps;
    size_t arrlen;
    size_t it = 0;
    int changed_mode = 0;

    keys = (char *)malloc(INIT_ARR_LEN * sizeof(keys[0]));
    if (keys == NULL) {
        fprintf(stderr, "Couldn't initialize keys array\n");
        goto fail;
    }
    tstamps = (struct timedesc *)malloc(INIT_ARR_LEN * sizeof(tstamps[0]));
    if (tstamps == NULL) {
        fprintf(stderr, "Couldn't initialize tstamps array\n");
        goto keys_fail;
    }
    arrlen = INIT_ARR_LEN;

    while ((matched = read_line(&key, &key_time.secs, &key_time.msecs)) != EOF) {
        if (matched != 3) {
            fprintf(stderr, "Scanf matched %d elements, expected 3\n", matched);
            goto fail;
        }
        if (config_optimize_mode(conf, mode)) {
            if (it == arrlen && !extend_arrs(&arrlen, &keys, &tstamps)) {
                goto tstamps_fail;
            }

            keys[it] = key;
            if (it > 0) {
                if (changed_mode || long_key_delay(prev_time, key_time)) {
                    tstamps[it] = tstamps[it-1];
                } else  {
                    tstamps[it] = tstamps_add(tstamps[it-1], tstamps_sub(key_time, prev_time));
                }
            } else {
                tstamps[it].secs = 0;
                tstamps[it].msecs = 0;
            }
            prev_time = key_time;
            ++it;
        }
        if (config_transition(conf, mode, key, &new_mode)) {
            changed_mode = 1;
            mode = new_mode;
        } else {
            changed_mode = 0;
        }
    }
    /*
     * Add terminating zero
     */
    {
        if (it == arrlen && !extend_arrs(&arrlen, &keys, &tstamps)) {
            goto tstamps_fail;
        }
        keys[it] = '\0';
        if (it > 0) {
            tstamps[it] = tstamps[it-1];
        } else {
            tstamps[it].secs = 0;
            tstamps[it].msecs = 0;
        }
        ++it;
    }
    *out_keys = keys;
    *out_tstamps = tstamps;
    *len = it;
    return;

tstamps_fail:
    free(tstamps);
keys_fail:
    free(keys);
fail:
    config_free(conf);
    exit(EXIT_FAILURE);
}

#define MAX_CHAIN_LEN 20
#define INF INT_MAX
#define MIN_REPETITIONS 5

struct result {
    int repetitions;
    int len;
    int idx;
};

int rescmp(const void *va, const void *vb) {
    const struct result *a = (const struct result *)va;
    const struct result *b = (const struct result *)vb;
    if (a->repetitions > b->repetitions) {
        return -1;
    } else if (a->repetitions < b->repetitions) {
        return 1;
    } else {
        return 0;
    }
}

int identify_patterns(size_t len, char *keys, int *a, int *l) {
    int it;
    int cl; // chain length
    int starts[MAX_CHAIN_LEN];

    struct result *ress;
    int rit = 0;
    int reps;

    ress = (struct result *)calloc(MAX_CHAIN_LEN*len, sizeof(*ress));
    if (ress == NULL) {
        return 0;
    }

    for (cl = 0; cl < MAX_CHAIN_LEN; ++cl) {
        starts[cl] = INF;
    }
    for (it = 1; it < len; ++it) {
        for (cl = 1; cl <= l[it] && cl <= MAX_CHAIN_LEN; ++cl) {
            if (it-1 < starts[cl-1]) {
                starts[cl-1] = it-1;
            }
        }
        for (; cl <= MAX_CHAIN_LEN; ++cl) {
            if (starts[cl-1] < INF) {
                reps = it - starts[cl-1];
                if (reps > MIN_REPETITIONS) {
                    ress[rit].repetitions = reps;
                    ress[rit].len = cl;
                    ress[rit].idx = a[it-1];
                    ++rit;
                }
            }
            starts[cl-1] = it;
        }
    }
    qsort(ress, rit, sizeof(*ress), rescmp);
    for (it = 0; it < rit; ++it) {
        printf("Repetitions: % 6d, length: %d, chain: %.*s\n",
                ress[it].repetitions,
                ress[it].len,
                ress[it].len,
                &keys[ress[it].idx]);
    }

    free(ress);
    return 1;
}

void find_patterns(config *conf) {
    char *keys;
    struct timedesc *tstamps;
    size_t len;
    size_t it;
    int *a;
    int *l;
    gen_key_time_data(conf, &keys, &tstamps, &len);

    a = scode(keys);
    if (a == NULL) {
        fprintf(stderr, "scode failure\n");
        goto scode_fail;
    }
    if (sarray(a, len) < 0) {
        fprintf(stderr, "sarray failure\n");
        goto sarray_fail;
    }
    l = lcp(a, keys, len);
    if (l == NULL) {
        fprintf(stderr, "lcp failure\n");
        goto lcp_fail;
    }

    if (!identify_patterns(len, keys, a, l)) {
        goto lcp_fail;
    }

    free(l);
    free(a);
    free(keys);
    free(tstamps);
    return;

lcp_fail:
    free(l);
sarray_fail:
    free(a);
scode_fail:
    free(keys);
    free(tstamps);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    config *conf = NULL;
    char *conf_path;
    int mode_logs;

    parse_args(argc, argv, &conf_path, &mode_logs);

    conf = config_read(conf_path);
    if (conf == NULL) {
        fprintf(stderr, "Error reading config from: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    if (mode_logs) {
        produce_mode_logs(conf);
    } else {
        find_patterns(conf);
    }
    config_free(conf);
    return EXIT_SUCCESS;
}
