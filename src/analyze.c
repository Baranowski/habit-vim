#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "keys.h"

void help(char *progname) {
    fprintf(stderr,
"Usage:\n"
"    %s config_file < log_file\n", progname);
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

void process_logs(config *conf) {
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

int main(int argc, char *argv[]) {
    config *conf = NULL;
    if (argc != 2) {
        help(argv[0]);
        return EXIT_FAILURE;
    }
    conf = config_read(argv[1]);
    if (conf == NULL) {
        fprintf(stderr, "Error reading config from: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    process_logs(conf);
    config_free(conf);
    return EXIT_SUCCESS;
}
