#include <stdio.h>
#include <stdlib.h>

#include "config.h"

void help(char *progname) {
    fprintf(stderr,
"Usage:\n"
"    %s config_file < log_file\n", progname);
}

void process_logs(config *conf) {
    int matched;
    char key;
    long secs, msecs;
    vmode_t mode = config_init_mode(conf);
    vmode_t new_mode;

    printf("%s: ", config_mode_name(conf, mode));
    while ((matched = scanf("%c %ld.%ld\n",&key, &secs, &msecs)) != EOF) {
        if (matched != 3) {
            fprintf(stderr, "Scanf matched %d elements, expected 3\n", matched);
            config_free(conf);
            exit(EXIT_FAILURE);
        }

        printf("%c", key);
        if (config_transition(conf, mode, key, &new_mode)) {
            printf("\n");
            mode = new_mode;
            printf("%s: ", config_mode_name(conf, mode));
        }
    }
    printf("\n");
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
    config_debug_print(conf);
    //process_logs(conf);
    config_free(conf);
    return EXIT_SUCCESS;
}
