#include "render/render.h"

#include <assert.h>
#include <stdio.h>
#include <wayland-util.h>
#include <wlr/render/gles2.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/util/edges.h>
#include <wlr/util/region.h>
#include <string.h>

#include "container.h"
#include "client.h"
#include "list_sets/container_stack_set.h"
#include "monitor.h"
#include "popup.h"
#include "root.h"
#include "server.h"
#include "tile/tileUtils.h"
#include "utils/gapUtils.h"
#include "tag.h"
#include "tagset.h"

static void render_stack(struct monitor *m, pixman_region32_t *output_damage);
static void scissor_output(struct wlr_output *output, pixman_box32_t *rect);
static void render_texture(struct wlr_output *wlr_output,
        pixman_region32_t *output_damage, struct wlr_texture *texture,
        const struct wlr_box *box, float alpha);

/* _box.x and .y are expected to be layout-local
   _box.width and .height are expected to be output-buffer-local */
void render_rect(struct monitor *m, pixman_region32_t *output_damage,
        const struct wlr_box *_box, const struct color color) {
    struct wlr_output *wlr_output = m->wlr_output;
    struct wlr_renderer *renderer = server.renderer;

    struct wlr_box box;
    memcpy(&box, _box, sizeof(struct wlr_box));

    pixman_region32_t damage;
    pixman_region32_init(&damage);
    pixman_region32_union_rect(&damage, &damage, box.x, box.y,
        box.width, box.height);
    pixman_region32_intersect(&damage, &damage, output_damage);
    bool damaged = pixman_region32_not_empty(&damage);
    if (!damaged) {
        pixman_region32_fini(&damage);
        return;
    }

    int nrects;
    pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
    for (int i = 0; i < nrects; ++i) {
        scissor_output(wlr_output, &rects[i]);
        float wlr_color[4];
        color_to_wlr_color(wlr_color, color);
        wlr_render_rect(renderer, &box, wlr_color,
            wlr_output->transform_matrix);
    }
}

static bool intersects_with_output(struct monitor *m,
        struct wlr_output_layout *output_layout, struct wlr_box *surface_box)
{
        /* Since the surface_box's x- and y-coordinates are already output local,
         * the x- and y-coordinates of this box need to be 0 for this function to
         * work correctly. */
        struct wlr_box output_box = {0};
        wlr_output_effective_resolution(m->wlr_output, &output_box.width, &output_box.height);

        struct wlr_box intersection;
        return wlr_box_intersection(&intersection, &output_box, surface_box);
}

/*
 * output_for_each_surface_iterator is a callback for rendering surfaces on an
 * output. Called by wlr_surface_for_each_surface in rendering order.
 *
 * Params:
 * - surface: The wlr_surface to be rendered on the output.
 * - sx: The x-coordinate of the surface relative to the output.
 * - sy: The y-coordinate of the surface relative to the output.
 * - user_data: A pointer to user-specific data to be used in the callback.
 */
static void output_for_each_surface_iterator(struct wlr_surface *surface, int sx, int sy, void *user_data)
{
    struct surface_iterator_data *data = user_data;

    if (!wlr_surface_has_buffer(surface))
        return;

    struct wlr_box surface_box = data->render_box;
    data->user_iterator(data->m, surface, &surface_box, data->user_data);
}

static void render_texture(struct wlr_output *wlr_output,
        pixman_region32_t *output_damage, struct wlr_texture *texture,
        const struct wlr_box *box, float alpha)
{
    struct wlr_renderer *renderer = server.renderer;

    pixman_region32_t damage;
    pixman_region32_init(&damage);
    pixman_region32_union_rect(&damage, &damage, box->x, box->y, box->width, box->height);
    pixman_region32_intersect(&damage, &damage, output_damage);
    if (!pixman_region32_not_empty(&damage)) {
        goto finish_damage;
    }

    int nrects;
    pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
    for (int i = 0; i < nrects; i++) {
        scissor_output(wlr_output, &rects[i]);
        wlr_render_texture(renderer, texture, wlr_output->transform_matrix,
                box->x, box->y, alpha);
    }

finish_damage:
    pixman_region32_fini(&damage);
}

/*
 * render_surface_iterator is a callback for rendering surfaces on an output.
 * It sets up the proper transformations and scales, then calls the render
 * function. Used by wlr_output_render_software_with_damage.
 */
static void render_surface_iterator(struct monitor *m, struct wlr_surface *surface,
        struct wlr_box *box, void *data)
{
    struct render_texture_data *render_data = data;
    float alpha = render_data->alpha;
    pixman_region32_t *output_damage = render_data->output_damage;

    struct wlr_texture *texture = wlr_surface_get_texture(surface);
    struct wlr_output *wlr_output = m->wlr_output;

    if (!texture)
        return;

    scale_box(box, wlr_output->scale);

    /* The client has a position in layout coordinates. If you have two displays,
     * one next to the other, both 1080p, a client on the rightmost display might
     * have layout coordinates of 2000,100. We need to translate that to
     * output-local coordinates, or (2000 - 1920). */
    double ox = box->x;
    double oy = box->y;
    wlr_output_layout_output_coords(server.output_layout, wlr_output, &ox, &oy);

    struct wlr_box obox = {
        /* We also have to apply the scale factor for HiDPI outputs. This is only
         * part of the puzzle, dwl does not fully support HiDPI. */
        .x = ox,
        .y = oy,
        .width = box->width,
        .height = box->height
    };

    render_texture(wlr_output, output_damage, texture, &obox, alpha);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_surface_send_frame_done(surface, &now);
}

static void render_surface_iterator_tmp(struct monitor *m, struct wlr_surface *surface,
        struct wlr_box *box, void *data)
{
    render_surface_iterator(m, surface, box, data);
}

static void
damage_surface_iterator(struct monitor *m, struct wlr_surface *surface,
        struct wlr_box *box, void *user_data)
{
    struct wlr_output *wlr_output = m->wlr_output;
    bool whole = *(bool *) user_data;
    struct wlr_box geom = *box;

    scale_box(&geom, wlr_output->scale);

    if (whole) {
        wlr_output_damage_add_box(m->damage, &geom);
    } else if (pixman_region32_not_empty(&surface->buffer_damage)) {
        pixman_region32_t damage;
        pixman_region32_init(&damage);
        wlr_surface_get_effective_damage(surface, &damage);

        wlr_region_scale(&damage, &damage, wlr_output->scale);
        if (ceil(wlr_output->scale) > surface->current.scale) {
            /* When scaling up a surface it'll become
               blurry, so we need to expand the damage
               region. */
            wlr_region_expand(&damage, &damage, ceil(wlr_output->scale) - surface->current.scale);
        }
        pixman_region32_translate(&damage, geom.x, geom.y);
        wlr_output_damage_add(m->damage, &damage);
        pixman_region32_fini(&damage);
    }
}

void output_damage_surface(struct monitor *m, struct wlr_surface *surface, struct wlr_box *geom, bool whole)
{
    assert(m != NULL);
    if (!m->wlr_output->enabled)
        return;

    double ox = geom->x, oy = geom->y;
    wlr_output_layout_output_coords(server.output_layout, m->wlr_output, &ox, &oy);

    struct surface_iterator_data data = {
        .user_iterator = damage_surface_iterator,
        .user_data = &whole,
        .m = m,
        .render_box = {
            .x = ox,
            .y = oy,
            .width = geom->width,
            .height = geom->height,
        },
    };

    wlr_surface_for_each_surface(surface, output_for_each_surface_iterator, &data);
}

static void scissor_output(struct wlr_output *output, pixman_box32_t *rect)
{
    struct wlr_renderer *renderer = server.renderer;

    struct wlr_box box = {
        .x = rect->x1,
        .y = rect->y1,
        .width = rect->x2 - rect->x1,
        .height = rect->y2 - rect->y1,
    };

    int output_width, output_height;
    wlr_output_transformed_resolution(output, &output_width, &output_height);
    enum wl_output_transform transform = wlr_output_transform_invert(output->transform);
    wlr_box_transform(&box, &box, transform, output_width, output_height);

    wlr_renderer_scissor(renderer, &box);
}

// TODO refactor the name it doesn't represent what this does perfectly
// returns the newly accquired hidden edges
static enum wlr_edges container_update_hidden_edges(struct container *con, struct wlr_box *borders, enum wlr_edges hidden_edges)
{
    struct monitor *m = container_get_monitor(con);

    enum wlr_edges containers_hidden_edges = WLR_EDGE_NONE;
    struct wlr_box con_geom = container_get_current_geom(con);
    // int border_width = container_get_border_width(con);
    // hide edges if needed
    if (hidden_edges & WLR_EDGE_LEFT) {
        if (con_geom.x == m->root->geom.x) {
            containers_hidden_edges |= WLR_EDGE_LEFT;
        }
    }
    if (hidden_edges & WLR_EDGE_RIGHT) {
        if (is_approx_equal(con_geom.x + con_geom.width, m->root->geom.x + m->root->geom.width, 3)) {
            containers_hidden_edges |= WLR_EDGE_RIGHT;
        }
    }
    if (hidden_edges & WLR_EDGE_TOP) {
        if (con_geom.y == m->root->geom.y) {
            containers_hidden_edges |= WLR_EDGE_TOP;
        }
    }
    if (hidden_edges & WLR_EDGE_BOTTOM) {
        if (is_approx_equal(con_geom.y + con_geom.height, m->root->geom.y + m->root->geom.height, 3)) {
            containers_hidden_edges |= WLR_EDGE_BOTTOM;
        }
    }

    container_set_hidden_edges(con, containers_hidden_edges);
    return containers_hidden_edges;
}

static void render_borders(struct container *con, struct monitor *m, pixman_region32_t *output_damage)
{
    // TODO: reimplement me
    if (!con->has_border)
        return;

    struct wlr_box *borders = (struct wlr_box[4]) {
        container_get_current_border_geom(con, WLR_EDGE_TOP),
        container_get_current_border_geom(con, WLR_EDGE_BOTTOM),
        container_get_current_border_geom(con, WLR_EDGE_LEFT),
        container_get_current_border_geom(con, WLR_EDGE_RIGHT),
    };

    enum wlr_edges hidden_edges = WLR_EDGE_NONE;
    struct tag *tag = monitor_get_active_tag(m);
    struct layout *lt = tag_get_layout(tag);
    if (lt->options->smart_hidden_edges) {
        if (tag->visible_con_set->tiled_containers->len <= 1) {
            hidden_edges = container_update_hidden_edges(con, borders, lt->options->hidden_edges);
        }
    } else {
        hidden_edges = container_update_hidden_edges(con, borders, lt->options->hidden_edges);
    }

    /* Draw window borders */
    struct container *sel = monitor_get_focused_container(m);
    const struct color color = (con == sel) ? lt->options->focus_color : lt->options->border_color;
    for (int i = 0; i < 4; i++) {
        if ((hidden_edges & (1 << i)) == 0) {
            struct wlr_box border = borders[i];
            double ox = border.x;
            double oy = border.y;
            wlr_output_layout_output_coords(server.output_layout, m->wlr_output, &ox, &oy);
            struct wlr_box obox = {
                .x = ox,
                .y = oy,
                .width = border.width,
                .height = border.height,
            };
            scale_box(&obox, m->wlr_output->scale);
            render_rect(m, output_damage, &obox, color);
        }
    }
}

void output_surface_for_each_surface(struct monitor *m,
        struct wlr_surface *surface, struct wlr_box obox,
        surface_iterator_func_t iterator, void *user_data) {

    struct surface_iterator_data data = {
        .user_iterator = iterator,
        .user_data = user_data,
        .m = m,
        .render_box = obox,
    };

    wlr_surface_for_each_surface(surface,
        output_for_each_surface_iterator, &data);
}

static void send_frame_done_func(struct wlr_surface *surface,
    int sx, int sy, void *data)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_surface_send_frame_done(surface, &now);
}

static void render_stack(struct monitor *m, pixman_region32_t *output_damage)
{
    /* Each subsequent window we render is rendered on top of the last. Because
     * our stacking list is ordered front-to-back, we iterate over it backwards. */
    struct tag *tag = monitor_get_active_tag(m);
    GPtrArray *stack_list = tag_get_complete_stack_copy(tag);
    for (int i = stack_list->len-1; i >= 0; i--) {
        struct container *con = g_ptr_array_index(stack_list, i);
        if (!container_viewable_on_monitor(m, con))
            continue;

        render_borders(con, m, output_damage);

        /* This calls our render function for each surface among the
         * xdg_surface's toplevel and popups. */

        struct wlr_surface *surface = get_wlrsurface(con->client);
        struct wlr_box obox = container_get_current_content_geom(con);

        struct render_texture_data render_data;
        render_data.alpha = con->alpha;
        render_data.output_damage = output_damage;
        output_surface_for_each_surface(m, surface, obox,
                render_surface_iterator, &render_data);
    }
    g_ptr_array_unref(stack_list);
}

static void render_popups(struct monitor *m, pixman_region32_t *output_damage)
{
    for (int i = 0; i < server.popups->len; i++) {
        struct xdg_popup *popup = g_ptr_array_index(server.popups, i);
        struct wlr_surface *surface = popup->xdg->base->surface;

        // popups are weird and require to use the dimensions of the surface
        // instead
        popup_set_width(popup, surface->current.width);
        popup_set_height(popup, surface->current.height);

        struct wlr_box obox = popup->geom;
        // printf("render popup: %i\n", i);
        // printf("obox.x: %i\n", obox.x);
        // printf("obox.y: %i\n", obox.y);
        // printf("obox.width: %i\n", obox.width);
        // printf("obox.height: %i\n", obox.height);

        struct render_texture_data render_data;
        render_data.alpha = 1.0f;
        render_data.output_damage = output_damage;
        output_surface_for_each_surface(m, surface, obox,
                render_surface_iterator, &render_data);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        wlr_surface_send_frame_done(surface, &now);
    }
}

static void clear_frame(
        struct monitor *m,
        struct color color,
        pixman_region32_t *damage)
{
    struct wlr_renderer *renderer = server.renderer;

    /* // debug stuff */
    /* float color2[4] = {0.4f, 0.1f, 0.0f, 1.0f}; */
    /* wlr_renderer_clear(renderer, color2); */

    float wlr_color[4];
    color_to_wlr_color(wlr_color, color);
    int nrects;
    pixman_box32_t *rects = pixman_region32_rectangles(damage, &nrects);
    for (int i = 0; i < nrects; i++) {
        scissor_output(m->wlr_output, &rects[i]);
        wlr_renderer_clear(renderer, wlr_color);
    }
}

void render_monitor(struct monitor *m, pixman_region32_t *damage)
{
    /* Begin the renderer (calls glViewport and some other GL sanity checks) */
    wlr_renderer_begin(server.renderer, m->wlr_output->width, m->wlr_output->height);

    clear_frame(m, m->root->color, damage);
    render_stack(m, damage);
    render_popups(m, damage);

    /* Hardware cursors are rendered by the GPU on a separate plane, and can be
     * moved around without re-rendering what's beneath them - which is more
     * efficient. However, not all hardware supports hardware cursors. For this
     * reason, wlroots provides a software fallback, which we ask it to render
     * here. wlr_cursor handles configuring hardware vs software cursors for you,
     * and this function is a no-op when hardware cursors are in use. */
    struct wlr_renderer *renderer = server.renderer;
    wlr_renderer_scissor(renderer, NULL);
    wlr_output_render_software_cursors(m->wlr_output, damage);

    /* Conclude rendering and swap the buffers, showing the final frame
     * on-screen. */
    wlr_renderer_end(renderer);

    wlr_output_commit(m->wlr_output);
}

void scale_box(struct wlr_box *box, float scale)
{
    box->x *= scale;
    box->y *= scale;
    box->width *= scale;
    box->height *= scale;
}
