#include <stdio.h>
#include <stdlib.h>

#include "analyze.h"

void help(char *progname) {
    fprintf(stderr,
"Usage:\n"
"    %s config_file < log_file\n", progname);
}

config * read_config(char *config_path) {
}

void process_logs(config *conf) {
    int matched;
    char key;
    long secs, msecs;
    char *mode = config_init_mode(conf);
    char *new_mode;

    printf("%s: ", mode);
    while ((matched = scanf("%c %ld.%ld\n",&key, &secs, &msecs)) != EOF) {
        if (matched != 3) {
            fprintf(stderr, "Scanf matched %d elements, expected 3\n", matched);
            free_config(conf);
            exit(EXIT_FAILURE);
        }

        printf("%c", key);
        if (config_transition(mode, key, &new_mode)) {
            printf("\n");
            mode = new_mode;
            printf("%s: ", mode);
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
    conf = read_config(argv[1]);
    process_logs(conf);
    free_config(conf);
    return EXIT_SUCCESS;
}
