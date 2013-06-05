#include "config.h"

#include <yaml.h>

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


#define parse_or_fail(parser, event) \
    do { \
        if (!yaml_parser_parse(parser, event)) { \
            fprintf(stderr, "Parsing error: %d\n", parser->error); \
            return 0; \
        } \
    } while(0)

#define wrap(parser, event, func, args...) \
    do { \
        if (!func(parser, event, args)) { \
            fprintf(stderr, "At position: %d:%d\n", \
                    event->start_mark.line, \
                    event->start_mark.column); \
            return 0; \
        } \
        return 1; \
    } while(0)

#define wrap_final(parser, event, func, args...) \
    do { \
        parse_or_fail(parser, event); \
        wrap(parser, event, func, args); \
        yaml_event_delete(event); \
    } while(0)


inline int expect_event(yaml_parser_t *parser, yaml_event_t *event, yaml_event_type_t etype) {
    if (event->type != etype) {
        fprintf(stderr, "Unexpected event type: %d; expected: %d\n",
            event->type, etype);
        return 0;
    }
    return 1;
}

inline int expect_value(yaml_parser_t *parser, yaml_event_t *event, char *val) {
    if (strcmp(event->data.scalar.value, val) != 0) {
        fprintf(stderr, "Unexpected value: %s; expected: %s\n",
                event->data.scalar.value, val);
        return 0;
    }
    return 1;
}

static int parse_top(config *conf, yaml_parser_t *parser, yaml_event_t *event) {
    wrap_final(parser, event,
         expect_event, YAML_STREAM_START_EVENT);

    wrap_final(parser, event,
         expect_event, YAML_DOCUMENT_START_EVENT);

    wrap_final(parser, event,
         expect_event, YAML_MAPPING_START_EVENT);

    parse_or_fail(parser, event);
    wrap(parser, event,
         expect_event, YAML_SCALAR_EVENT);
    wrap(parser, event,
         expect_value, "Modes");
    yaml_event_delete(event);

    parse_modes(conf);

    wrap_final(parser, event,
               expect_event, YAML_MAPPING_END_EVENT);

    wrap_final(parser, event,
               expect_event, YAML_DOCUMENT_END_EVENT);

    wrap_final(parser, event,
               expect_event, YAML_STREAM_END_EVENT);

    return 1;
}

#undef wrap
#undef wrap_final
#undef parse_or_fail


config *config_read(char *path) {
    yaml_parser_t parser;
    yaml_event_t event;
    config *result;

    FILE *cf = fopen(path, "r");
    if (cf == NULL) {
        fprintf(stderr, "Cannot open for reading: %s\n", path);
        goto fopen_fail;
    }

    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Cannot initialize YAML parser\n");
        goto fopen_fail;
    }

    yaml_parser_set_input_file(&parser, cf);

    result = (config *)malloc(sizeof(*result));

    if (!parse_top(result, &parser, &event)) {
        goto parse_fail;
    }

    yaml_parser_delete(&parser);
    fclose(cf);
    return result;

parse_fail:
    free(result);
    yaml_parser_delete(&parser);
    fclose(cf);
fopen_fail:
    return NULL;
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

