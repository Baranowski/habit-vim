#ifndef _CONFIG_H
#define _CONFIG_H

typedef struct config {
} config;

config *read_config(char *path);
void config_free(config *);
char *config_init_mode(config *conf);
int config_transition(config *, char *old_mode, char key, char **new_mode);

#endif
