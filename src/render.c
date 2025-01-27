#include "render.h"

#include <wayland-server.h>
#include <wayland-server-core.h>

#include "container.h"
#include "client.h"
#include "tag.h"
#include "monitor.h"
#include "layout.h"
#include "list_sets/container_stack_set.h"
#include "root.h"
#include "tagset.h"

static void surface_handle_destroy(struct wl_listener *listener, void *data)
{
    struct scene_surface *surface = wl_container_of(listener, surface, destroy);
    for (int i = 0; i < BORDER_COUNT; i++) {
        struct wlr_scene_rect *border = surface->borders[i];
        wlr_scene_node_destroy(&border->node);
    }
    wlr_scene_node_destroy(&surface->scene_surface->buffer->node);
    wl_list_remove(&surface->destroy.link);
    free(surface);
}

static void surface_handle_commit(struct wl_listener *listener, void *data)
{
    struct scene_surface *surface = wl_container_of(listener, surface, commit);

    struct client *c = wlr_surface_get_client(surface->wlr);
    struct container *con = c->con;
    container_update_border_color(con);
}

static void surface_handle_new_subsurface(struct wl_listener *listener, void *data)
{
}

void server_handle_new_surface(struct wl_listener *listener, void *data)
{
    struct server *server = wl_container_of(listener, server, new_surface);
    struct wlr_surface *wlr_surface = data;

    struct scene_surface *surface = calloc(1, sizeof(struct scene_surface));
    surface->wlr = wlr_surface;
    printf("wlr_surface: %p\n", wlr_surface);
    surface->commit.notify = surface_handle_commit;
    wl_signal_add(&wlr_surface->events.commit, &surface->commit);
    surface->destroy.notify = surface_handle_destroy;
    wl_signal_add(&wlr_surface->events.destroy, &surface->destroy);

    surface->new_subsurface.notify = surface_handle_new_subsurface;;
    wl_signal_add(&wlr_surface->events.new_subsurface, &surface->new_subsurface);

    surface->surface_tree = wlr_scene_tree_create(server->scene_tiled);

    surface->scene_surface =
        wlr_scene_surface_create(surface->surface_tree, wlr_surface);
    // TODO: this doesn't work for popups
    // wlr_scene_node_set_position(&surface->scene_surface->buffer->node, 100, 100);
    // wlr_scene_node_set_position(&surface->scene_surface->buffer->node, 200, 200);

    for (int i = 0; i < BORDER_COUNT; i++) {
        surface->borders[i] =
            wlr_scene_rect_create(surface->surface_tree,
                    0, 0, (float[4]){ 1.0f, 0.0f, 0.0f, 1 });
    }

    wlr_surface->data = surface;
}

struct client *wlr_surface_get_client(struct wlr_surface *wlr_surface)
{
    return wlr_surface->data;
}
