#ifndef _ANALYZE_H
#define _ANALYZE_H

typedef struct config {
} config;

config *read_config(char *path);
free_config(config *);
char *config_init_mode(config *conf);
int config_transition(char *old_mode, char key, char **new_mode);

#endif
