#include "parseConfig.h"
#include <wordexp.h>
#include <wlr/util/log.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <lualib.h>
#include <lua.h>
#include <libgen.h>
#include <lauxlib.h>
#include "utils/gapUtils.h"
#include "translationLayer.h"
#include "tile/tileUtils.h"
#include "root.h"
#include "lib/actions/actions.h"
#include "stringop.h"

const char *config_paths[] = {
    "$HOME/.config/juliawm/",
    "$XDG_CONFIG_HOME/juliawm/",
    "/etc/juliawm/",
};

bool sloppyFocus;
int borderPx;
float borderColor[4];
float focusColor[4];
float overlayColor[4];
float textColor[4];
float selOverlayColor[4];
float selTextColor[4];

struct wlr_list tagNames;
struct rule rules[MAXLEN];

//MonitorRule monrules[MAXLEN];

int repeatRate;
int repeatDelay;
int inner_gap;
int outer_gap;

struct wlr_list tagNames;
char *termcmd;
Key *keys = NULL;
Key *buttons = NULL;

char *get_config_layout_path()
{
    return get_config_file("layouts");
}

char *get_config_dir(const char *file)
{
    char *abs_file = get_config_file(file);
    return dirname(abs_file);
}

char *get_config_file(const char *file)
{
    for (size_t i = 0; i < LENGTH(config_paths); ++i) {
        char *path = strdup(config_paths[i]);
        expand_path(&path);
        path = realloc(path, strlen(path) + strlen(file));
        join_path(path, file);
        if (file_exists(path))
            return path;
        free(path);
    }
    return NULL;
}

void append_to_lua_path(lua_State *L, const char *path)
{
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    const char * curr_path = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    char *path_var = calloc(1,
            strlen(curr_path) + 1 + strlen(path) + strlen("/?.lua"));

    strcpy(path_var, curr_path);
    strcat(path_var, ";");
    strcat(path_var, path);
    join_path(path_var, "/?.lua");
    lua_pushstring(L, path_var);
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
    free(path_var);
}

int update_config(lua_State *L)
{
    // init
    init_error_file();
    char *config_path = get_config_dir("init.lua");
    // add current path as the priority and all alternatives non prioritized
    append_to_lua_path(L, config_path);
    int j = -1;
    for (int i = 0; i < LENGTH(config_paths); i++) {
        char *path = strdup(config_paths[i]);

        expand_path(&path);
        append_to_lua_path(L, path);

        if (path_compare(path, config_path) == 0)
            j = i;
        if (j < 0)
            continue;
        printf("go\n");
        if (load_config(L, path)) {
            wlr_log(WLR_ERROR, "file didn't load correctly");
            continue;
        }
        break;
    }

    free(config_path);

    sloppyFocus = get_config_bool(L, "sloppyFocus");
    borderPx = get_config_int(L, "borderPx");

    /* gaps */
    inner_gap = get_config_int(L, "innerGap");
    outer_gap = get_config_int(L, "outerGap");
    configure_gaps(&inner_gap, &outer_gap);

    /* appearance */
    get_config_float_arr(L, root.color, "rootColor");
    get_config_float_arr(L, borderColor, "borderColor");
    get_config_float_arr(L, focusColor, "focusColor");
    get_config_float_arr(L, overlayColor, "overlayColor");
    get_config_float_arr(L, textColor, "textColor");
    get_config_float_arr(L, selOverlayColor, "overlayColor");
    get_config_float_arr(L, selTextColor, "textColor");

    wlr_list_init(&tagNames);
    get_config_str_arr(L, &tagNames, "tagNames");
    get_config_rule_arr(L, rules, "rules");

    /* monitors */
    //getConfigMonRuleArr(monrules, "monrules");

    /* keyboard */
    repeatRate = get_config_int(L, "repeatRate");
    repeatDelay = get_config_int(L, "repeatDelay");
    defaultLayout = get_config_layout(L, "defaultLayout");
    prev_layout = (struct layout){.symbol = "", .funcId = 0};

    /* commands */
    termcmd = get_config_str(L, "termcmd");
    get_config_key_arr(L, keys, "keys");
    get_config_key_arr(L, buttons, "buttons");

    close_error_file();
    return 0;
}

int reloadConfig(lua_State *L)
{
    for (int i = 0; i < tagNames.length; i++)
        free(wlr_list_pop(&tagNames));
    wlr_list_finish(&tagNames);

    // TODO: only one monitor will reload his tagset
    unsigned int focusedTag = selected_monitor->tagset->focusedTag;
    unsigned int selTags = selected_monitor->tagset->selTags[0];
    destroy_tagset(selected_monitor->tagset);
    update_config(L);
    selected_monitor->tagset = create_tagset(&tagNames, focusedTag, selTags);

    // reconfigure clients
    struct client *c = NULL;
    wl_list_for_each(c, &clients, link) {
        c->bw = borderPx;
    }

    lua_pushboolean(L, true);
    arrange_this(L);
    return 0;
}
