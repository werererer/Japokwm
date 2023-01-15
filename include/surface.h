#ifndef SURFACE_H
#define SURFACE_H
#include <wayland-server.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>

struct surface {
    struct wlr_surface *surface;
    struct wl_list slink;
    struct wlr_box geom;
};

#endif /* SURFACE_H */
