#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "keys.h"

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
            if (it == arrlen) {
                void *tmp;

                arrlen *= 2;
                tmp = realloc(keys, arrlen * sizeof(keys[0]));
                if (tmp == NULL) {
                    fprintf(stderr, "Realloc failed.\n");
                    goto tstamps_fail;
                }
                keys = tmp;
                tmp = realloc(tstamps, arrlen * sizeof(tstamps[0]));
                if (tmp == NULL) {
                    fprintf(stderr, "Realloc failed.\n");
                    goto tstamps_fail;
                }
                tstamps = tmp;
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

void find_patterns(config *conf) {
    char *keys;
    struct timedesc *tstamps;
    size_t len;
    size_t it;
    gen_key_time_data(conf, &keys, &tstamps, &len);
    for (it = 0; it < len; ++it) {
        printf("%c %ld.%06ld\n", keys[it], tstamps[it].secs, tstamps[it].msecs);
    }
    free(keys);
    free(tstamps);
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
