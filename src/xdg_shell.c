#include "xdg_shell.h"

#include <stdlib.h>
#include <wlr/types/wlr_xdg_decoration_v1.h>
#include <wlr/util/edges.h>

#include "client.h"
#include "monitor.h"
#include "server.h"
#include "utils/coreUtils.h"
#include "container.h"
#include "tile/tileUtils.h"
#include "tag.h"
#include "subsurface.h"

static void destroyxdeco(struct wl_listener *listener, void *data);
static void getxdecomode(struct wl_listener *listener, void *data);

static void destroyxdeco(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *wlr_deco = data;
    Decoration *d = wlr_deco->data;

    wl_list_remove(&d->destroy.link);
    wl_list_remove(&d->request_mode.link);
    free(d);
}

static void getxdecomode(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *wlr_deco = data;
    wlr_xdg_toplevel_decoration_v1_set_mode(wlr_deco,
            WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
}

void create_notify_xdg(struct wl_listener *listener, void *data)
{
    printf("create notify xdg\n");
    /* This event is raised when wlr_xdg_shell receives a new xdg surface from a
     * client, either a toplevel (application window) or popup. */
    struct wlr_xdg_surface *xdg_surface = data;

    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL)
        return;

    union surface_t surface;
    surface.xdg = xdg_surface;
    /* Allocate a Client for this surface */
    struct client *c = xdg_surface->data = create_client(XDG_SHELL, surface);
    xdg_surface->surface->data = c;

    /* Tell the client not to try anything fancy */
    wlr_xdg_toplevel_set_tiled(c->surface.xdg->toplevel, WLR_EDGE_TOP |
            WLR_EDGE_BOTTOM | WLR_EDGE_LEFT | WLR_EDGE_RIGHT);

    /* Listen to the various events it can emit */
    LISTEN(&xdg_surface->surface->events.map, &c->associate, map_request);
    LISTEN(&xdg_surface->surface->events.commit, &c->commit, commit_notify);
    LISTEN(&xdg_surface->events.configure, &c->configure, configure_notify);
    LISTEN(&xdg_surface->events.ack_configure, &c->ack_configure, ack_configure);
    LISTEN(&xdg_surface->surface->events.unmap, &c->dissociate, unmap_notify);
    LISTEN(&xdg_surface->events.destroy, &c->destroy, destroy_notify);
    LISTEN(&xdg_surface->surface->events.new_subsurface, &c->new_subsurface, handle_new_subsurface);

    LISTEN(&xdg_surface->toplevel->events.set_title, &c->set_title, client_handle_set_title);
    LISTEN(&xdg_surface->toplevel->events.set_app_id, &c->set_app_id, client_handle_set_app_id);

    LISTEN(&xdg_surface->events.new_popup, &c->new_popup, client_handle_new_popup);

    create_container(c, server_get_selected_monitor(), true);
}

void destroy_notify(struct wl_listener *listener, void *data)
{
    // printf("DESTROY\n");
    // struct client *c = wl_container_of(listener, c, destroy);
    //
    // wl_list_remove(&c->map.link);
    // wl_list_remove(&c->commit.link);
    // wl_list_remove(&c->configure.link);
    // wl_list_remove(&c->ack_configure.link);
    // wl_list_remove(&c->unmap.link);
    // wl_list_remove(&c->destroy.link);
    // wl_list_remove(&c->new_subsurface.link);
    //
    // wl_list_remove(&c->set_title.link);
    // wl_list_remove(&c->set_app_id.link);
    //
    // wl_list_remove(&c->new_popup.link);
    //
    // destroy_container(c->con);
    // destroy_client(c);
}

void map_request(struct wl_listener *listener, void *data)
{
    /* Called when the surface is mapped, or ready to display on-screen. */
    struct client *c = wl_container_of(listener, c, associate);
    struct container *con = c->con;
    struct tag *tag = get_tag(con->tag_id);

    add_container_to_tile(con);
    arrange();
    tag_focus_most_recent_container(tag);
}

void unmap_notify(struct wl_listener *listener, void *data)
{
    printf("Unmap\n");
    /* Called when the surface is unmapped, and should no longer be shown. */
    struct client *c = wl_container_of(listener, c, dissociate);

    struct container *con = c->con;
    remove_container_from_tile(con);

    arrange();
    tag_this_focus_most_recent_container();
}

void createxdeco(struct wl_listener *listener, void *data)
{
    struct wlr_xdg_toplevel_decoration_v1 *wlr_deco = data;
    Decoration *d = wlr_deco->data = calloc(1, sizeof(*d));

    wl_signal_add(&wlr_deco->events.request_mode, &d->request_mode);
    d->request_mode.notify = getxdecomode;
    wl_signal_add(&wlr_deco->events.destroy, &d->destroy);
    d->destroy.notify = destroyxdeco;

    getxdecomode(&d->request_mode, wlr_deco);
}
