#include "keys.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

struct key_mapping {
    char key;
    char *hreadable;
};

static const struct key_mapping keys_map[] =
    { { ' ',    "<Space>"                  }
    , { '\t',   "<Tab>"                    }
    , { '\x1b', "<Esc>"                    }
    , { '\n',   "<Ret>"                    }
    , { '\x7f', "<Backspace>"              }
    , { '\0',   NULL                       }
    };

#define MAX_CTRL_COMBO 26
#define MIN_CTRL_COMBO 1

static const char *ctrl_hreadable_tpl = "<C-x>";
#define CTRL_COMBO_SUBST_POS 3

//TODO: handle ctrl combos
char key_from_hreadable(const char *hreadable) {
    if (strlen(hreadable) == 1) {
        return hreadable[0];
    } else {
        const struct key_mapping *it;
        for (it = keys_map;
                 strcmp(hreadable, it->hreadable) != 0 &&
                 it->hreadable != NULL;
             ++it) {
            ;
        }
        if (it->hreadable == NULL) {
            return '\0';
        } else {
            return it->key;
        }
    }
}

char *hreadable_from_key(char key) {
    char *result = NULL;
    if (isprint(key) && !isspace(key)) {
        result = (char *)calloc(2, sizeof(result[0]));
        result[0] = key;
        //result[1] has been set to 0 by calloc
    } else if (MIN_CTRL_COMBO <= key && key <= MAX_CTRL_COMBO) {
        size_t len = strlen(ctrl_hreadable_tpl);
        result = (char *)calloc(len + 1, sizeof(result[0]));
        strncpy(result, ctrl_hreadable_tpl, len);
        result[CTRL_COMBO_SUBST_POS] = 'A' + key - MIN_CTRL_COMBO;

    } else {
        const struct key_mapping *it;
        for (it = keys_map;
                it->hreadable != NULL &&
                it->key != key;
             ++it) {
            ;
        }
        if (it->hreadable != NULL) {
            size_t len = strlen(it->hreadable);
            result = (char *)calloc(len + 1, sizeof(result[0]));
            strncpy(result, it->hreadable, len);
        }
    }
    return result;
}
