extern "C" {
#include "config.h"
}

#include <stdlib.h>
#include <string.h>
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

#include <yaml-cpp/yaml.h>
#include <string>

vmode_t find_mode_by_name(config *conf, const char *name) {
    int i;
    for (i = 0;
             i < conf->num_modes &&
             strcmp(conf->modes[i].name, name) != 0;
         ++i) {
        ;
    }
    if (i == conf->num_modes) {
        i = MODE_INVALID;
    }
    return i;
}

int parse_mode_transitions(config *conf, int mid, const YAML::Node &transs) {
    if (transs.IsMap()) {
        fprintf(stderr, "Config file: '%s/Transitions' is not a map.\n", conf->modes[mid].name);
        return 0;
    }
    size_t trans_total = 0;
    for (YAML::const_iterator it = transs.begin();
         it != transs.end(); ++it) {
        trans_total += it->second.size();
    }
    conf->modes[mid].transitions = (struct transition *)
        calloc(trans_total + 1,
               sizeof(conf->modes[mid].transitions[0]));
    
    size_t trans_current = 0;
    for (YAML::const_iterator it = transs.begin();
         it != transs.end(); ++it) {
        std::string nm_name = it->first.as<std::string>();
        vmode_t nmid = find_mode_by_name(conf, nm_name.c_str());
        if (nmid == MODE_INVALID) {
            fprintf(stderr, "Config file: Unrecognized mode name: %s\n", nm_name.c_str());
            return 0;
        }

        //TODO: make sure the config syntax is correct
        for (int i = 0; i < it->second.size(); ++i) {
            conf->modes[mid].transitions[trans_current].nmid = nmid;
            conf->modes[mid].transitions[trans_current].key =
                it->second[i].as<std::string>()[0];
            ++trans_current;
        }
    }
    conf->modes[mid].transitions[trans_current].nmid = MODE_INVALID;
    return 1;
}

int parse_modes(config *conf, const YAML::Node &modes_node) {
    if (!modes_node.IsSequence()) {
        fprintf(stderr, "Config file: 'Modes' is not a sequence.\n");
        return 0;
    }
    conf->num_modes = modes_node.size();
    conf->modes = (struct mode *)calloc(modes_node.size(),
                                        sizeof(conf->modes[0]));
    for (int i = 0; i < conf->num_modes; ++i) {
        conf->modes[i].name = NULL;
        conf->modes[i].transitions = NULL;
    }

    /* Parse names */
    for (int i = 0; i < modes_node.size(); ++i) {
        try {
            std::string name = modes_node[i]["Name"].as<std::string>().c_str();
            conf->modes[i].name = (char *)malloc(name.length() + 1);
            strncpy(conf->modes[i].name, name.c_str(), name.length() + 1);
        } catch (YAML::Exception &e) {
            fprintf(stderr, "Error parsing YAML: %s\n", e.what());
            goto fail;
        }
    }

    /* Parse transitions */
    for (int i = 0; i < modes_node.size(); ++i) {
        try {
            YAML::Node transs = modes_node[i]["Transitions"];
            if (!parse_mode_transitions(conf, i, transs)) {
                goto fail;
            }

        } catch (YAML::Exception &e) {
            fprintf(stderr, "Error parsing YAML: %s\n", e.what());
            goto fail;
        }
    }
    return 1;

fail:
    for (int i = 0; i < conf->num_modes; ++i) {
        free(conf->modes[i].name);
        free(conf->modes[i].transitions);
    }
    return 0;
}

config *config_read(char *path) {
    config *result = (config *)malloc(sizeof(config *));
    try {
        const YAML::Node root = YAML::LoadFile(path);
        if (!root.IsMap()) {
            fprintf(stderr, "Root of the config file is not a map.\n");
            goto fail;
        }
        if (!parse_modes(result, root["Modes"])) {
            goto fail;
        }
    } catch (YAML::Exception &e) {
        fprintf(stderr, "Error parsing YAML: %s\n", e.what());
        goto fail;
    }
    return result;

fail:
    free(result);
    return NULL;
}

