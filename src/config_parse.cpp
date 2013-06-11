extern "C" {
#include "config.h"
#include "keys.h"
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    if (!transs.IsMap()) {
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
            char keycode;
            const char *hreadable;

            hreadable = it->second[i].as<std::string>().c_str();
            keycode = key_from_hreadable(hreadable);

            if (keycode == '\0') {
                fprintf(stderr, "Config file: Unrecognized key: %s\n",
                        hreadable);
                return 0;
            }
            conf->modes[mid].transitions[trans_current].nmid = nmid;
            conf->modes[mid].transitions[trans_current].key = keycode;
            ++trans_current;
        }
    }
    conf->modes[mid].transitions[trans_current].nmid = MODE_INVALID;
    return 1;
}

int parse_optimize(config *conf, const YAML::Node &opt_node) {
    if (!opt_node.IsScalar()) {
        fprintf(stderr, "Config file: 'Optimize' is not a scalar.\n");
        return 0;
    }
    const char *mode_name = opt_node.as<std::string>().c_str();
    vmode_t result = find_mode_by_name(conf, mode_name);
    if (result == MODE_INVALID) {
        fprintf(stderr, "Config file: 'Optimize': unknown mode: %s\n", mode_name);
        return 0;
    }
    conf->opt_mode = result;
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
        if (!parse_optimize(result, root["Optimize"])) {
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
