#include <glib.h>

#include "monitor.h"
#include "container.h"
#include "tag.h"
#include "utils/coreUtils.h"
#include "server.h"
#include "client.h"

static void assert_wlr_box_equal(struct wlr_box box1, struct wlr_box box2) {
    g_assert_cmpint(box1.x, ==, box2.x);
    g_assert_cmpint(box1.y, ==, box2.y);
    g_assert_cmpint(box1.width, ==, box2.width);
    g_assert_cmpint(box1.height, ==, box2.height);
}

void test_container_box_to_content_geometry()
{
    init_server();
    struct wlr_box box = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100,
    };
    struct wlr_box content_box = {
        .x = 10,
        .y = 10,
        .width = 80,
        .height = 80,
    };
    struct wlr_box left_border_hidden_content_box = {
        .x = 0,
        .y = 10,
        .width = 90,
        .height = 80,
    };
    struct wlr_box right_border_hidden_content_box = {
        .x = 10,
        .y = 10,
        .width = 90,
        .height = 80,
    };
    struct wlr_box top_border_hidden_content_box = {
        .x = 10,
        .y = 0,
        .width = 80,
        .height = 90,
    };
    struct wlr_box bottom_border_hidden_content_box = {
        .x = 10,
        .y = 10,
        .width = 80,
        .height = 90,
    };
    struct monitor m = {0};
    struct client c = {
        .sticky_tags = bitset_create(),
    };

    struct container *con = create_container(&c, &m, true);
    container_set_border_width(con, direction_value_uniform(10));
    struct wlr_box result = container_box_to_content_geometry(con, box);
    assert_wlr_box_equal(result, content_box);

    con->hidden_edges = WLR_EDGE_LEFT;
    result = container_box_to_content_geometry(con, box);
    assert_wlr_box_equal(result, left_border_hidden_content_box);

    con->hidden_edges = WLR_EDGE_RIGHT;
    result = container_box_to_content_geometry(con, box);
    assert_wlr_box_equal(result, right_border_hidden_content_box);

    con->hidden_edges = WLR_EDGE_TOP;
    result = container_box_to_content_geometry(con, box);
    assert_wlr_box_equal(result, top_border_hidden_content_box);

    con->hidden_edges = WLR_EDGE_BOTTOM;
    result = container_box_to_content_geometry(con, box);
    assert_wlr_box_equal(result, bottom_border_hidden_content_box);

    con->hidden_edges = WLR_EDGE_NONE;
    con->has_border = false;
    result = container_box_to_content_geometry(con, box);
    assert_wlr_box_equal(result, box);

    destroy_container(con);
    bitset_destroy(c.sticky_tags);
}

void test_container_get_current_border_geom()
{
    init_server();
    struct wlr_box box = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100,
    };
    struct wlr_box border_left = {
        .x = 0,
        .y = 0,
        .width = 10,
        .height = 100,
    };
    struct wlr_box border_right = {
        .x = 90,
        .y = 0,
        .width = 10,
        .height = 100,
    };
    struct wlr_box border_top = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 10,
    };
    struct wlr_box border_bottom = {
        .x = 0,
        .y = 90,
        .width = 100,
        .height = 10,
    };
    struct monitor m;
    struct client c = {
        .sticky_tags = bitset_create(),
    };

    struct container *con = create_container(&c, &m, true);
    container_set_current_geom(con, box);
    container_set_border_width(con, direction_value_uniform(10));

    struct wlr_box left = container_get_current_border_geom(con, WLR_EDGE_LEFT);
    struct wlr_box right = container_get_current_border_geom(con, WLR_EDGE_RIGHT);
    struct wlr_box top = container_get_current_border_geom(con, WLR_EDGE_TOP);
    struct wlr_box bottom = container_get_current_border_geom(con, WLR_EDGE_BOTTOM);

    assert_wlr_box_equal(left, border_left);
    assert_wlr_box_equal(right, border_right);
    assert_wlr_box_equal(top, border_top);
    assert_wlr_box_equal(bottom, border_bottom);
}

void test_container_content_geometry_to_box()
{
    init_server();
    struct wlr_box box = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100,
    };
    struct wlr_box content_box = {
        .x = 10,
        .y = 10,
        .width = 80,
        .height = 80,
    };
    struct wlr_box left_border_hidden_content_box = {
        .x = 0,
        .y = 10,
        .width = 90,
        .height = 80,
    };
    struct wlr_box right_border_hidden_content_box = {
        .x = 10,
        .y = 10,
        .width = 90,
        .height = 80,
    };
    struct wlr_box top_border_hidden_content_box = {
        .x = 10,
        .y = 0,
        .width = 80,
        .height = 90,
    };
    struct wlr_box bottom_border_hidden_content_box = {
        .x = 10,
        .y = 10,
        .width = 80,
        .height = 90,
    };
    struct monitor m;
    struct client c = {
        .sticky_tags = bitset_create(),
    };
    struct container *con = create_container(&c, &m, true);
    container_set_border_width(con, direction_value_uniform(10));
    struct wlr_box result = container_content_geometry_to_box(con, content_box);
    g_assert_cmpint(result.x, ==, box.x);
    g_assert_cmpint(result.y, ==, box.y);
    g_assert_cmpint(result.width, ==, box.width);
    g_assert_cmpint(result.height, ==, box.height);

    con->hidden_edges = WLR_EDGE_LEFT;
    result = container_content_geometry_to_box(con, left_border_hidden_content_box);
    assert_wlr_box_equal(result, box);

    con->hidden_edges = WLR_EDGE_RIGHT;
    result = container_content_geometry_to_box(con, right_border_hidden_content_box);
    assert_wlr_box_equal(result, box);

    con->hidden_edges = WLR_EDGE_TOP;
    result = container_content_geometry_to_box(con, top_border_hidden_content_box);
    assert_wlr_box_equal(result, box);

    con->hidden_edges = WLR_EDGE_BOTTOM;
    result = container_content_geometry_to_box(con, bottom_border_hidden_content_box);
    assert_wlr_box_equal(result, box);

    free(con);
    bitset_destroy(c.sticky_tags);
}

void test_container_current_geom()
{
    init_server();

    struct wlr_box box = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100,
    };
    struct wlr_box content_box = {
        .x = 10,
        .y = 10,
        .width = 80,
        .height = 80,
    };

    struct client c = {
        .sticky_tags = bitset_create(),
    };

    struct monitor m;
    struct container *con = create_container(&c, &m, true);

    container_set_current_geom(con, box);

    container_set_border_width(con, direction_value_uniform(10));

    struct wlr_box result = container_get_current_geom(con);
    g_assert_cmpint(result.x, ==, box.x);
    g_assert_cmpint(result.y, ==, box.y);
    g_assert_cmpint(result.width, ==, box.width);
    g_assert_cmpint(result.height, ==, box.height);
    struct wlr_box content_result = container_get_current_content_geom(con);
    g_assert_cmpint(content_result.x, ==, content_box.x);
    g_assert_cmpint(content_result.y, ==, content_box.y);
    g_assert_cmpint(content_result.width, ==, content_box.width);
    g_assert_cmpint(content_result.height, ==, content_box.height);

    free(con);
    bitset_destroy(c.sticky_tags);
}

void test_container_floating_content_geom()
{
    init_server();

    struct wlr_box box = {
        .x = 0,
        .y = 0,
        .width = 100,
        .height = 100,
    };
    struct wlr_box content_box = {
        .x = 10,
        .y = 10,
        .width = 80,
        .height = 80,
    };

    struct client c = {
        .sticky_tags = bitset_create(),
    };

    struct monitor m;
    struct container *con = create_container(&c, &m, true);

    container_set_floating_geom(con, box);

    container_set_border_width(con, direction_value_uniform(10));

    struct wlr_box result = container_get_floating_geom(con);
    g_assert_cmpint(result.x, ==, box.x);
    g_assert_cmpint(result.y, ==, box.y);
    g_assert_cmpint(result.width, ==, box.width);
    g_assert_cmpint(result.height, ==, box.height);

    result = container_get_floating_content_geom(con);
    g_assert_cmpint(result.x, ==, content_box.x);
    g_assert_cmpint(result.y, ==, content_box.y);
    g_assert_cmpint(result.width, ==, content_box.width);
    g_assert_cmpint(result.height, ==, content_box.height);

    free(con);
    bitset_destroy(c.sticky_tags);
}

void test_visible_on()
{
    /* struct wlr_list tag_names; */
    /* wlr_list_init(&tag_names); */
    /* wlr_list_push(&tag_names, "1"); */
    /* wlr_list_push(&tag_names, "2"); */
    /* wlr_list_push(&tag_names, "3"); */
    /* wlr_list_push(&tag_names, "4"); */

    /* struct layout lt; */
    /* lt.options.arrange_by_focus = false; */
    /* struct wlr_list tags; */
    /* create_tags(&tags, &tag_names); */

    /* struct monitor m0; */
    /* struct monitor m1; */

    /* struct tag *ws0 = tags.items[0]; */
    /* struct tag *ws1 = tags.items[1]; */
    /* ws0->m = &m0; */
    /* ws1->m = &m1; */

    /* struct client c; */
    /* struct container con = { */
    /*     .client = &c */
    /* }; */

    /* con.m = &m0; */
    /* con.client->tag_id = ws0->id; */
    /* con.hidden = false; */
    /* ck_assert_int_eq(visible_on(&con, ws0), true); */

    /* con.m = &m1; */
    /* con.client->tag_id = ws0->id; */
    /* con.hidden = false; */
    /* ck_assert_int_eq(visible_on(&con, ws0), false); */

    /* con.m = &m0; */
    /* con.client->tag_id = ws1->id; */
    /* con.hidden = false; */
    /* ck_assert_int_eq(visible_on(&con, ws1), false); */

    /* con.m = &m1; */
    /* con.client->tag_id = ws1->id; */
    /* con.hidden = true; */
    /* ck_assert_int_eq(visible_on(&con, ws1), false); */
}

void test_exist_on()
{
    /* struct wlr_list tag_names; */
    /* wlr_list_init(&tag_names); */
    /* wlr_list_push(&tag_names, "1"); */
    /* wlr_list_push(&tag_names, "2"); */
    /* wlr_list_push(&tag_names, "3"); */
    /* wlr_list_push(&tag_names, "4"); */

    /* struct layout lt; */
    /* lt.options.arrange_by_focus = false; */
    /* struct wlr_list tags; */
    /* create_tags(&tags, &tag_names, &lt); */

    /* struct monitor m0; */
    /* struct monitor m1; */

    /* struct tag *ws0 = tags.items[0]; */
    /* struct tag *ws1 = tags.items[1]; */

    /* ws0->m = &m0; */
    /* ws1->m = &m1; */

    /* struct client c; */
    /* struct container con = { */
    /*     .client = &c */
    /* }; */

    /* con.m = &m0; */
    /* con.client->tag_id = ws0->id; */
    /* con.hidden = true; */
    /* ck_assert_int_eq(exist_on(&con, ws0), true); */

    /* con.m = &m1; */
    /* con.client->tag_id = ws0->id; */
    /* con.hidden = false; */
    /* ck_assert_int_eq(exist_on(&con, ws0), false); */

    /* con.m = &m0; */
    /* con.client->tag_id = ws1->id; */
    /* con.hidden = false; */
    /* ck_assert_int_eq(exist_on(&con, ws1), false); */

    /* con.m = &m1; */
    /* con.client->tag_id = ws1->id; */
    /* con.hidden = false; */
    /* ck_assert_int_eq(exist_on(&con, ws1), true); */
}

void focus_on_hidden_stack_test()
{
    // TODO fix this unittest
}

void focus_container_test()
{
}

void get_position_in_container_stack_crash_test()
{
    get_position_in_container_focus_stack(NULL);
}

void get_focused_container_crash_test()
{
    /* get_focused_container(NULL); */

    /* struct monitor m; */
    /* m.tagset = -1; */
    /* get_focused_container(&m); */

    /* m.tagset = 700; */
    /* get_focused_container(&m); */
}

#define PREFIX "container"
#define add_test(func) g_test_add_func("/"PREFIX"/"#func, func)
int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
    g_test_init(&argc, &argv, NULL);

    // add_test(test_container_box_to_content_geometry);
    // add_test(test_container_get_current_border_geom);
    // add_test(test_container_content_geometry_to_box);
    // add_test(test_container_current_geom);
    // add_test(test_container_floating_content_geom);
    // add_test(test_visible_on);
    //
    return g_test_run();
}
