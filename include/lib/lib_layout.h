#ifndef LIB_LAYOUT_H
#define LIB_LAYOUT_H

#include <lua.h>
#include <lauxlib.h>

struct layout;

void create_lua_layout(struct layout *layout);
void lua_init_layout(struct layout *layout);
void lua_load_layout();

// functions
// methods
int lib_set_layout(lua_State *L);
int lib_set_master_layout_data(lua_State *L);
int lib_set_linked_layouts_ref(lua_State *L);
int lib_set_resize_data(lua_State *L);
int lib_set_resize_function(lua_State *L);
// setter
int lib_set_default_layout(lua_State *L);
// getter
int lib_layout_get_direction(lua_State *L);
int lib_layout_get_layout_data(lua_State *L);
int lib_layout_get_n(lua_State *L);
int lib_layout_get_o_layout_data(lua_State *L);
int lib_layout_get_resize_data(lua_State *L);

#endif /* LIB_LAYOUT_H */
