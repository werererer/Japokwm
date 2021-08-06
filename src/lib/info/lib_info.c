#include "lib/info/lib_info.h"

#include <lauxlib.h>

#include "container.h"
#include "monitor.h"
#include "server.h"
#include "tile/tileUtils.h"

int lib_get_this_container_count(lua_State *L)
{
    struct monitor *m = selected_monitor;
    struct tagset *tagset = monitor_get_active_tagset(m);

    int i = get_slave_container_count(tagset) + 1;
    lua_pushinteger(L, i);
    return 1;
}

int lib_this_container_position(lua_State *L)
{
    struct monitor *m = selected_monitor;
    struct container *sel = get_focused_container(m);

    int position = get_position_in_container_stack(sel);
    lua_pushinteger(L, position);
    return 1;
}

int lib_get_next_empty_workspace(lua_State *L)
{
    enum wlr_direction dir = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    int id = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    printf("lib_get_next_empty_workspace: %i\n", id);

    struct workspace *ws;
    switch (dir) {
        case WLR_DIRECTION_LEFT:
            ws = get_prev_empty_workspace(server.workspaces, id);
            break;
        case WLR_DIRECTION_RIGHT:
            ws = get_next_empty_workspace(server.workspaces, id);
            break;
        default:
            ws = get_workspace(id);
    }

    int ws_id = (ws) ? ws->id : id;
    lua_pushinteger(L, ws_id);
    return 1;
}

int lib_get_nmaster(lua_State *L)
{
    struct layout *lt = get_layout_in_monitor(selected_monitor);
    lua_pushinteger(L, lt->nmaster);
    return 1;
}

int lib_get_workspace(lua_State *L)
{
    struct monitor *m = selected_monitor;
    lua_pushinteger(L, m->tagset->selected_ws_id);
    return 1;
}

int lib_get_container_under_cursor(lua_State *L)
{
    struct seat *seat = input_manager_get_default_seat();
    struct wlr_cursor *wlr_cursor = seat->cursor->wlr_cursor;

    struct container *con = xy_to_container(wlr_cursor->x, wlr_cursor->y);
    int pos = get_position_in_container_stack(con);
    lua_pushinteger(L, pos);
    return 1;
}

int lib_get_root_area(lua_State *L)
{
    struct monitor *m = selected_monitor;
    struct root *root = m->root;
    lua_createtable(L, 1, 0);
    lua_pushinteger(L, root->geom.x);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, root->geom.y);
    lua_rawseti(L, -2, 2);
    lua_pushinteger(L, root->geom.width);
    lua_rawseti(L, -2, 3);
    lua_pushinteger(L, root->geom.height);
    lua_rawseti(L, -2, 4);
    return 1;
}

int lib_is_container_not_in_limit(lua_State *L)
{
    struct layout *lt = get_layout_in_monitor(selected_monitor);

    struct wlr_fbox geom = lua_togeometry(L);
    lua_pop(L, 1);

    bool not_in_limit = is_resize_not_in_limit(&geom, &lt->options.layout_constraints);
    return not_in_limit;
}

int lib_is_container_not_in_master_limit(lua_State *L)
{
    struct layout *lt = get_layout_in_monitor(selected_monitor);

    struct wlr_fbox geom = lua_togeometry(L);
    lua_pop(L, 1);

    bool not_in_limit = is_resize_not_in_limit(&geom, &lt->options.master_constraints);
    return not_in_limit;
}