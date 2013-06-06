#include "config.h"

#include <stdlib.h>
#include <stdio.h>

vmode_t config_init_mode(config *conf) {
    return conf->init_mode;
}

char *config_mode_name(config *conf, vmode_t mode) {
    return conf->modes[mode].name;
}

int config_transition(config *conf, vmode_t old_mode, char key, vmode_t *new_mode) {
    struct transition *it = conf->modes[old_mode].transitions;
    for (; it->nmid != MODE_INVALID; ++it) {
        if (it->key == key) {
            *new_mode = it->nmid;
            return 1;
        }
    }
    return 0;
}

static void mode_free(struct mode* md) {
    free(md->name);
    free(md->transitions);
}

void config_free(config *conf) {
    vmode_t mode_it;
    for (mode_it = 0; mode_it < conf->num_modes; ++mode_it) {
        mode_free(&conf->modes[mode_it]);
    }
    free(conf);
}

void config_debug_print(config *conf) {
    vmode_t mid;
    int i;
    for (mid = 0; mid < conf->num_modes; ++mid) {

        printf("%d: %s\n", mid, conf->modes[mid].name);
        struct transition *transs = conf->modes[mid].transitions;
        for (i = 0; transs[i].nmid != MODE_INVALID; ++i) {
            printf("    %c -> %d\n", transs[i].key, transs[i].nmid);
        }
    }
}
