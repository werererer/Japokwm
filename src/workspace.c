#include "workspace.h"

#include <assert.h>
#include <string.h>
#include <wayland-server.h>
#include <wlr/util/log.h>
#include <wlr/types/wlr_cursor.h>

#include "ipc-server.h"
#include "monitor.h"
#include "server.h"
#include "tile/tileUtils.h"
#include "utils/parseConfigUtils.h"

static struct wlr_list workspaces;

struct workspace *create_workspace(const char *name, size_t id, struct layout lt)
{
    struct workspace *ws = malloc(sizeof(struct workspace));
    ws->name = name;

    for (int i = 0; i < 2; i++)
        ws->layout[i] = get_default_layout();

    // fill layout stack with reasonable values
    push_layout(ws->layout, lt);
    push_layout(ws->layout, lt);

    ws->id = id;
    ws->m = NULL;
    return ws;
}

void destroy_workspace(struct workspace *ws)
{
    free(ws);
}

void focus_top_container(struct workspace *ws, enum focus_actions a)
{
    // focus_stack should not be changed while iterating
    struct container *con;
    bool container_found = false;
    wl_list_for_each(con, &focus_stack, flink) {
        if (visibleon(con, ws)) {
            container_found = true;
            break;
        }
    }
    if (container_found)
        focus_container(con, a);
}

void init_workspaces()
{
    wlr_list_init(&workspaces);
}

void create_workspaces(struct wlr_list tagNames, struct layout default_layout)
{
    for (int i = 0; i < tagNames.length; i++) {
        struct workspace *ws = create_workspace(tagNames.items[i], i, default_layout);
        wlr_list_push(&workspaces, ws);
    }
}

void destroy_workspaces()
{
    for (int i = 0; i < workspace_count(); i++)
        destroy_workspace(wlr_list_pop(&workspaces));
    wlr_list_finish(&workspaces);
}

bool is_workspace_occupied(struct workspace *ws)
{
    assert(ws);

    return ws->m ? true : false;
}

bool existon(struct container *con, struct workspace *ws)
{
    if (!con || !ws)
        return false;
    if (con->floating)
        return true;
    if (con->m != ws->m)
        return false;

    struct client *c = con->client;

    if (!c)
        return false;

    if (c->type == LAYER_SHELL)
        return true;
    if (c->sticky)
        return true;

    return c->ws == ws;
}

bool workspace_has_clients(struct workspace *ws)
{
    if (!ws)
        return 0;

    int count = 0;

    struct client *c;
    wl_list_for_each(c, &clients, link)
        if (c->ws == ws)
            count++;

    return count > 0;
}

bool hiddenon(struct container *con, struct workspace *ws)
{
    if (!con || !ws)
        return false;
    if (con->floating)
        return false;
    if (con->m != ws->m)
        return false;
    if (!con->hidden)
        return false;

    struct client *c = con->client;

    if (!c)
        return false;
    // LayerShell based programs are visible on all workspaces
    if (c->type == LAYER_SHELL)
        return false;
    if (c->sticky)
        return true;

    return c->ws == ws;
}

bool visibleon(struct container *con, struct workspace *ws)
{
    if (!con || !ws)
        return false;
    if (con->floating)
        return true;
    if (con->m != ws->m)
        return false;
    if (con->hidden)
        return false;

    struct client *c = con->client;

    if (!c)
        return false;

    // LayerShell based programs are visible on all workspaces
    if (c->type == LAYER_SHELL)
        return true;
    if (c->sticky)
        return true;

    return c->ws == ws;
}

int workspace_count()
{
    return workspaces.length;
}

int get_workspace_container_count(struct workspace *ws)
{
    assert(ws);

    int i = 0;
    struct container *con;
    wl_list_for_each(con, &containers, mlink) {
        if (visibleon(con, ws))
            i++;
    }
    return i;
}

bool is_workspace_empty(struct workspace *ws)
{
    return get_workspace_container_count(ws) == 0;
}

struct workspace *find_next_unoccupied_workspace(struct workspace *ws)
{
    for (size_t i = ws ? ws->id : 0; i < workspace_count(); i++) {
        struct workspace *w = get_workspace(i);
        if (!w)
            break;
        if (!is_workspace_occupied(w))
            return w;
    }
    return NULL;
}

struct workspace *get_workspace(size_t i)
{
    if (i >= workspaces.length)
        return NULL;

    return workspaces.items[i];
}

struct workspace *get_next_empty_workspace(size_t i)
{
    struct workspace *ws = NULL;
    for (int j = i; j < workspaces.length; j++) {
        ws = get_workspace(j);
        if (is_workspace_empty(ws))
            break;
    }

    return ws;
}

struct workspace *get_prev_empty_workspace(size_t i)
{
    if (i >= workspaces.length)
        return NULL;

    struct workspace *ws = NULL;
    for (int j = i; j >= 0; j--) {
        ws = get_workspace(j);
        if (is_workspace_empty(ws))
            break;
    }

    return ws;
}

void workspace_assign_monitor(struct workspace *ws, struct monitor *m)
{
    ws->m = m;
}

void set_selected_layout(struct workspace *ws, struct layout layout)
{
    if (!ws)
        return;

    if (strcmp(ws->name, "") == 0) {
        wlr_log(WLR_ERROR, "ERROR: tag not initialized");
        return;
    }
    push_layout(ws->layout, layout);
}

void focus_next_unoccupied_workspace(struct monitor *m, struct workspace *ws)
{
    struct workspace *w = find_next_unoccupied_workspace(ws);
    focus_workspace(m, w);
}

void focus_workspace(struct monitor *m, struct workspace *ws)
{
    if (!m || !ws)
        return;
    assert(m->damage != NULL);

    // focus the workspace in the monitor it appears if such a monitor exist
    // and is not the selected one
    if (is_workspace_occupied(ws) && ws->m != selected_monitor) {
        center_mouse_in_monitor(ws->m);
        set_selected_monitor(ws->m);
        focus_workspace(ws->m, ws);
        return;
    }

    struct container *con;
    wl_list_for_each(con, &sticky_stack, stlink) {
        con->client->ws = ws;
    }

    ipc_event_workspace();

    // unset old workspace
    if (m->ws[0] && !workspace_has_clients(m->ws[0]))
        m->ws[0]->m = NULL;

    m->ws[0] = ws;
    ws->m = m;

    // TODO is wlr_output_damage_whole better? because of floating windows
    root_damage_whole(m->root);
}

void copy_layout_from_selected_workspace()
{
    for (int i = 0; i < workspaces.length; i++) {
        struct workspace *ws = workspaces.items[i];
        struct layout *dest_lt = &ws->layout[0];
        struct layout *dest_prev_lt = &ws->layout[1];
        struct layout *src_lt = &selected_monitor->ws[0]->layout[0];

        if (dest_lt == src_lt)
            continue;

        *dest_lt = copy_layout(src_lt);
        *dest_prev_lt = copy_layout(src_lt);
    }
}

void load_default_layout(lua_State *L, struct workspace *ws)
{
    load_layout(L, ws, server.default_layout.name, server.default_layout.symbol);
}

// TODO refactor this function
void set_layout(lua_State *L, struct workspace *ws)
{
    if (server.layout_set.layout_sets_ref <= 0) {
        printf("layout sets ref NULL\n");
        return;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, server.layout_set.layout_sets_ref);
    if (!lua_is_index_defined(L, server.layout_set.key)) {
        lua_pop(L, 1);
        return;
    }
    lua_get_layout_set_element(L, server.layout_set.key);

    lua_rawgeti(L, -1, server.layout_set.lua_layout_index);
    lua_rawgeti(L, -1, 1);
    const char *layout_symbol = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
    const char *layout_name = luaL_checkstring(L, -1);
    lua_pop(L, 1);
    lua_pop(L, 1);

    lua_pop(L, 1);

    lua_pop(L, 1);
    load_layout(L, ws, layout_name, layout_symbol);
}


void load_layout(lua_State *L, struct workspace *ws, const char *layout_name, const char *layout_symbol)
{
    struct layout *lt = &ws->layout[0];
    lt->name = layout_name;
    lt->symbol = layout_symbol;

    char *config_path = get_config_file("layouts");
    char file[NUM_CHARS] = "";
    strcpy(file, "");
    join_path(file, config_path);
    join_path(file, layout_name);
    join_path(file, "init.lua");
    if (config_path)
        free(config_path);

    if (!file_exists(file))
        return;

    if (luaL_loadfile(L, file)) {
        lua_pop(L, 1);
        return;
    }
    lua_call_safe(L, 0, 0, 0);
}

void push_workspace(struct workspace *ws_stack[static 2], struct workspace *ws)
{
    ws_stack[1] = ws_stack[0];
    focus_workspace(selected_monitor, ws);
}
