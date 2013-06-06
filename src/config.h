#ifndef _CONFIG_H
#define _CONFIG_H

typedef unsigned char vmode_t;

#define MAX_MODES 255
#define MODE_INVALID MAX_MODES

struct transition {
    char key;
    vmode_t nmid; // New Mode ID
};

struct mode {
    char *name;
    struct transition *transitions;
};

typedef struct config {
    struct mode *modes;
    vmode_t init_mode;
    vmode_t num_modes;
} config;

void config_debug_print(config *conf);
config *config_read(char *path);
void config_free(config *);
vmode_t config_init_mode(config *conf);
char *config_mode_name(config *, vmode_t);
int config_transition(config *, vmode_t old_mode, char key, vmode_t *new_mode);

#endif
