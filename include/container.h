#ifndef CONTAINER_H
#define CONTAINER_H

#include <wlr/types/wlr_box.h>

#include "client.h"

enum focus_actions {
    FOCUS_NOOP,
    FOCUS_LIFT,
};

struct container {
    /* monitor containers */
    struct wl_list mlink;
    /* client containers */
    struct wl_list clink;
    /* container stack */
    struct wl_list slink;
    /* container focus_stack */
    struct wl_list flink;
    /* layer shell based clients */
    struct wl_list llink;

    /* layout-relative, includes border */
    struct wlr_box geom;
    struct client *client;

    struct monitor *m;
    bool floating;
    bool hidden;
    // if position -1 it is floating
    int position;
    int clientPosition;
    int textPosition;
    int resize;
    float scale;
};

struct container *create_container(struct client *c, struct monitor *m);
void destroy_container(struct container *con);

struct container *next_container(struct monitor *m);
struct container *selected_container(struct monitor *m);
struct container *xytocontainer(double x, double y);
struct container *get_container(struct monitor *m, int i);
struct container *first_container(struct monitor *m);
struct container *last_container(struct monitor *m);
struct wlr_box get_absolute_box(struct wlr_box box, struct wlr_fbox b);
struct wlr_fbox get_relative_box(struct wlr_box box, struct wlr_box b);
void add_container_to_monitor(struct monitor *m, struct container *con);
void applybounds(struct container *con, struct wlr_box bbox);
void applyrules(struct container *con);
void focus_container(struct monitor *m, struct container *con, enum focus_actions a);
/* Find the topmost visible client (if any) at point (x, y), including
 * borders. This relies on stack being ordered from top to bottom. */
bool existon(struct container *con, struct monitor *m);
bool hiddenon(struct container *con, struct monitor *m);
bool visibleon(struct container *con, struct monitor *m);
void focus_top_container(struct monitor *m, enum focus_actions a);
void lift_container(struct container *con);
void remove_container_from_monitor(struct monitor *m, struct container *con);
#endif /* CONTAINER_H */