#ifndef ROOT_H
#define ROOT_H

#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_output_layout.h>
#include "color.h"

struct workspace;

struct root {
    /* window area(area where windows can tile) */
    struct wlr_box geom;
    struct monitor *m;
    struct color color;
};

struct root *create_root(struct monitor *m, struct wlr_box geom);
void destroy_root(struct root *root);
/* set the are where windows can be placed in respect to layershell based
 * programs which occupie space
 * The resulting root area is relative to outputs*/
void set_root_color(struct root *root, struct color color);
void set_root_geom(struct root *root, struct wlr_box geom);
void root_damage_whole(struct root *root);
void set_bars_visible(struct workspace *ws, bool visible, enum wlr_direction direction);
bool get_bars_visible(struct workspace *ws);
void toggle_bars_visible(struct workspace *ws, enum wlr_direction direction);

#endif /* ROOT_H */
