#ifndef LIB_CONFIG_H
#define LIB_CONFIG_H

#include "options.h"
#include <lauxlib.h>
#include <lua.h>

// init
void lua_load_options(lua_State *L);
void lua_init_options(struct options *options);
void create_lua_options(struct options *options);
struct options *check_options(lua_State *L, int narg);

int lib_add_mon_rule(lua_State *L);
int lib_add_rule(lua_State *L);
int lib_bind_key(lua_State *L);
int lib_create_layout_set(lua_State *L);
int lib_create_tags(lua_State *L);
int lib_reload(lua_State *L);
int lib_set_arrange_by_focus(lua_State *L);
int lib_set_automatic_tag_naming(lua_State *L);
int lib_set_border_color(lua_State *L);
int lib_set_buttons(lua_State *L);
int lib_set_entry_position_function(lua_State *L);
int lib_set_entry_focus_position_function(lua_State *L);
int lib_set_float_border_width(lua_State *L);
int lib_set_focus_color(lua_State *L);
int lib_set_hidden_edges(lua_State *L);
int lib_set_hide_edge_borders(lua_State *L);
int lib_set_inner_gaps(lua_State *L);
int lib_set_layout_constraints(lua_State *L);
int lib_set_master_constraints(lua_State *L);
int lib_set_mod(lua_State *L);
int lib_set_outer_gaps(lua_State *L);
int lib_set_repeat_delay(lua_State *L);
int lib_set_repeat_rate(lua_State *L);
int lib_set_resize_direction(lua_State *L);
int lib_set_root_color(lua_State *L);
int lib_set_sloppy_focus(lua_State *L);
int lib_set_smart_hidden_edges(lua_State *L);
int lib_set_tile_border_width(lua_State *L);
int lib_set_default_layout(lua_State *L);

int lib_get_sloppy_focus(lua_State *L);
int lib_get_inner_gaps(lua_State *L);

#endif /* LIB_CONFIG_H */
