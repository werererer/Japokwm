#ifndef COREUTILS
#define COREUTILS

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <lua.h>
#include <lauxlib.h>
#include "options.h"

/* a 2d array type that means that the lists have to be filled with other
 * GPtrArray variables
 * */
typedef GPtrArray GPtrArray2D;

/* macros */
//NOLINTNEXTLINE
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define END(A)                  ((A) + LENGTH(A))
/* number of chars a string should contain */
#define ARR_STRING_LENGTH(X) strlen(X) + 2*(strlen("[]") + NUM_DIGITS)

// those constants are arbitrary and are bound to change
#define MAXLEN 15
#define NUM_CHARS 64
#define NUM_DIGITS 9
#define PERCENT_TO_INTEGER_SCALE 10000
#define INVALID_POSITION -1
#define INVALID_TAG_ID -1
#define SWAP(a, b) do { \
    typeof(a) tmp = a; \
    a = b; \
    b = tmp; \
} while (0)

#define MIN_CONTAINER_WIDTH 30
#define MIN_CONTAINER_HEIGHT 30
#define LISTEN(E, L, H)         wl_signal_add((E), ((L)->notify = (H), (L)))

#define foreach(item, array)\
    for(int keep = 1, count = 0, size = LENGTH(array); keep && count < size; keep = 1, count++)\
        for(item = array[count]; keep; keep = 0)\

extern struct lua_State *L;

bool dir_exists(const char *path);
bool file_exists(const char *path);
char last_char(const char *str);
// returns exactly the same values as strcmp
int path_compare(const char *path1, const char *path2);
void join_path(char **base, const char *file);
// works exactly like c++'s std::lower_bound
int lower_bound(const void *key, const void *base,
        size_t nmemb, size_t size,
        int (*compar)(const void *, const void *));


void debug_print(const char *fmt, ...);
void print_trace();

int cross_sum(int n, int base);

/*
 * create a lua table that looks like this:
 * {
 *   {
 *     {0, 0, 1, 1}
 *   }
 * }
 */
void lua_get_default_layout_data(lua_State *L);

void lua_get_default_resize_function(lua_State *L);

/*
 * create a lua table that looks like this:
 * {
 *   {
 *     {0, 0, 1, 1}
 *   },
 *   {
 *     {0, 0, 1, 0.5},
 *     {0, 0.5, 1, 0.5},
 *   },
 *   {
 *     {0, 0, 1, 0.333},
 *     {0, 0.333, 1, 0.333},
 *     {0, 0.5, 1, 0.333},
 *   },
 * }
 */
void lua_get_default_master_layout_data(lua_State *L);
/*
 * create a lua table that looks like this:
 * {
 *   {
 *     {2, 3, 4, 5, 6, 7, 8, 9}
 *   }
 * }
 */
void lua_get_default_resize_data(lua_State *L);

void list_clear(GPtrArray *array, void (*destroy_func)(void *));
void wlr_list_cat(GPtrArray *dest, GPtrArray *src);
void list_insert(GPtrArray *array, int i, void *item);

/* return true on success and false on failure */
bool list_remove(GPtrArray *array, int (*compare)(const void *, const void *), const void *cmp_to);
bool remove_in_composed_list(GPtrArray *array, int (*compare)(const void *, const void *), void *cmp_to);
int find_in_composed_list(GPtrArray *lists,
        int (*compare)(const void *, const void *), const void *cmp_to);
GPtrArray *find_list_in_composed_list(GPtrArray *arrays,
        int (*compare)(const void *, const void *), const void *cmp_to);

GPtrArray *list_insert_into_relative_position(GPtrArray *lists, int index, void *item);

GPtrArray2D *lists_copy_structure(GPtrArray2D *src);

// compare pointers and return 0 if they are equal and 1 otherwise
int cmp_int(const void *ptr1, const void *ptr2);
int cmp_ptr(const void *ptr1, const void *ptr2);
int cmp_str(const void *s1, const void *s2);
int cmp_strptr(const void *s1, const void *s2);

// like lua_ref but override the old value if *ref > 0
void lua_ref_safe(lua_State *L, int t, int *ref);

/* a composed list is just a list consisting of lists so that if an index i is
 * given it returns the same value as if all the lists where concatenated */
void set_in_composed_list(GPtrArray *arrays, int i, void *value);
void *get_in_composed_list(GPtrArray *arrays, int i);
GPtrArray *get_list_at_i_in_composed_list(GPtrArray *arrays, int i);
void delete_from_composed_list(GPtrArray *arrays, int i);

int length_of_list(GPtrArray *array);
int length_of_composed_list(GPtrArray *array);
int relative_index_to_absolute_index(int i, int j, int length);

void *get_relative_item_in_list(GPtrArray *array, int i, int j);
void *get_relative_item_in_composed_list(GPtrArray *arrays, int i, int j);

int exec(const char *cmd);
bool is_approx_equal(double a, double b, double error_range);

int lua_idx_to_c_idx(int lua_idx);
int c_idx_to_lua_idx(int c_idx);

int scale_percent_to_integer(float percent);
float scale_integer_to_percent(int integer);

#endif /* COREUTILS */
