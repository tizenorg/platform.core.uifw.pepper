#include "x11-internal.h"

static uint8_t
x11_get_depth_of_visual(xcb_screen_t *screen, xcb_visualid_t id)
{
    xcb_depth_iterator_t i;
    xcb_visualtype_iterator_t j;

    i = xcb_screen_allowed_depths_iterator(screen);
    for (; i.rem; xcb_depth_next(&i))
    {
        j = xcb_depth_visuals_iterator(i.data);
        for (; j.rem; xcb_visualtype_next(&j))
        {
            if (j.data->visual_id == id)
                return i.data->depth;
        }
    }
    return 0;
}

static void
x11_output_visual_iterate(void *o)
{
    x11_output_t *output = o;

    xcb_screen_t     *screen;
    xcb_visualtype_t *visual_type = NULL;    /* the returned visual type */

    screen = output->connection->screen;
    if (screen)
    {
        xcb_depth_iterator_t depth_iter;

        depth_iter = xcb_screen_allowed_depths_iterator(screen);
        for (; depth_iter.rem; xcb_depth_next(&depth_iter))
        {
            xcb_visualtype_iterator_t visual_iter;

            visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            for (; visual_iter.rem; xcb_visualtype_next(&visual_iter))
            {
                visual_type = visual_iter.data;
                printf("visual %d, class: %d, bits per rgb value: %d, colormap_entries %d, r:%#010x, g:%#010x, b:%#010x, depth %d\n",
                       visual_type->visual_id,
                       visual_type->_class,
                       visual_type->colormap_entries,
                       visual_type->bits_per_rgb_value,
                       visual_type->red_mask,
                       visual_type->green_mask,
                       visual_type->blue_mask,
                       x11_get_depth_of_visual(screen, visual_type->visual_id));
            }
        }
    }
}

static void
x11_output_destroy(void *o)
{
    x11_output_t            *output;
    pepper_x11_connection_t *conn;

    if (!o)
    {
        PEPPER_ERROR("x11:output:%s: output is null\n", __FUNCTION__);
        return ;
    }

    output = o;
    conn = output->connection;

    wl_signal_emit(&output->destroy_signal, output);

    xcb_destroy_window(conn->xcb_connection, output->window);

    wl_list_remove(&output->link);

    pepper_free(output);
}

static int32_t
x11_output_get_subpixel_order(void *o)
{
    x11_output_t *output = o;
    return output->subpixel;
}

static const char *
x11_output_get_maker_name(void *o)
{
    PEPPER_IGNORE(o);
    return "pepper_x11";
}

static const char *
x11_output_get_model_name(void *o)
{
    PEPPER_IGNORE(o);
    return "pepper_x11";
}

static int
x11_output_get_mode_count(void *o)
{
    PEPPER_IGNORE(o);

    /* There's only one available mode in x11 backend which is also the current mode. */
    return 1;
}

static void
x11_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    x11_output_t *output = o;

    if (index != 0)
        return;

    mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    mode->w = output->w;
    mode->h = output->h;
    mode->refresh = 60000;
}

static pepper_bool_t
x11_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    x11_output_t *output = o;

    if (mode->w <= 0 || mode->h <= 0)
        return PEPPER_FALSE;

    if (mode->refresh != 60000)
        return PEPPER_FALSE;

    if (output->w != mode->w || output->h != mode->h)
    {
        output->w = mode->w;
        output->h = mode->h;

        /* TODO: Handle resize here. */

        wl_signal_emit(&output->mode_change_signal, output);
    }

    return PEPPER_TRUE;
}

static void
x11_output_add_destroy_listener(void *o, struct wl_listener *listener)
{
    x11_output_t *output = o;
    wl_signal_add(&output->destroy_signal, listener);
}

static void
x11_output_add_mode_change_listener(void *o, struct wl_listener *listener)
{
    x11_output_t *output = o;
    wl_signal_add(&output->mode_change_signal, listener);
}

static const pepper_output_interface_t x11_output_interface =
{
    x11_output_destroy,
    x11_output_add_destroy_listener,
    x11_output_add_mode_change_listener,

    x11_output_get_subpixel_order,
    x11_output_get_maker_name,
    x11_output_get_model_name,

    x11_output_get_mode_count,
    x11_output_get_mode,
    x11_output_set_mode,
};

static void
handle_connection_destroy(struct wl_listener *listener, void *data)
{
    x11_output_t *output = wl_container_of(listener, output, conn_destroy_listener);
    x11_output_destroy(output);
}

PEPPER_API pepper_output_t *
pepper_x11_output_create(pepper_x11_connection_t *connection, int32_t w, int32_t h)
{
    static const char   *window_name = "PePPer Compositor";
    static const char   *class_name  = "pepper-1\0PePPer Compositor";

    pepper_output_t     *base;
    x11_output_t        *output;

    output = pepper_calloc(1, sizeof(x11_output_t));
    if (!output)
    {
        PEPPER_ERROR("x11:output:%s: memory allocation failed", __FUNCTION__);
        return NULL;
    }

    output->connection = connection;
    output->w = w;
    output->h = h;

    /* Hard-Coded: subpixel order to horizontal RGB. */
    output->subpixel = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;

    /* Hard-Coded: scale value to 1. */
    output->scale = 1;

    /* create X11 window */
    {
        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t values[2] = {
                connection->screen->white_pixel,
                0
        };
        xcb_size_hints_t hints;

        output->window = xcb_generate_id(connection->xcb_connection);
        xcb_create_window(connection->xcb_connection,
                          XCB_COPY_FROM_PARENT,
                          output->window,
                          connection->screen->root,
                          0,    /* X position of top-left corner of window */
                          0,    /* Y position of top-left corner of window */
                          w*output->scale,
                          h*output->scale,
                          0,    /* width of windows' border */
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          connection->screen->root_visual,
                          mask,
                          values);


        /* cannot resize */
        memset(&hints, 0, sizeof(hints));
        hints.flags = WM_NORMAL_HINTS_MAX_SIZE | WM_NORMAL_HINTS_MIN_SIZE;
        hints.min_width  = hints.max_width  = w*output->scale;
        hints.min_height = hints.max_height = h*output->scale;
        xcb_change_property(connection->xcb_connection,
                            XCB_PROP_MODE_REPLACE,
                            output->window,
                            connection->atom.wm_normal_hints,
                            connection->atom.wm_size_hints,
                            32,
                            sizeof(hints) / 4,
                            (uint8_t *)&hints);

        /* set window name */
        xcb_change_property(connection->xcb_connection, XCB_PROP_MODE_REPLACE,
                            output->window,
                            connection->atom.net_wm_name,
                            connection->atom.utf8_string, 8,
                            strlen(window_name), window_name);
        xcb_change_property(connection->xcb_connection, XCB_PROP_MODE_REPLACE,
                            output->window,
                            connection->atom.wm_class,
                            connection->atom.string, 8,
                            strlen(class_name), class_name);

        xcb_map_window(connection->xcb_connection, output->window);

        if (connection->use_xinput)
            x11_window_input_property_change(connection->xcb_connection,
                                             output->window);

        wl_list_insert(&connection->outputs, &output->link);

        xcb_flush(connection->xcb_connection);
    }

    wl_signal_init(&output->destroy_signal);
    wl_signal_init(&output->mode_change_signal);

    /*x11_output_visual_iterate(output);*/

    base = pepper_compositor_add_output(connection->compositor,
                                        &x11_output_interface,
                                        output);
    if (!base)
    {
        PEPPER_ERROR("x11:output:%s: pepper_compositor_add_output failed\n", __FUNCTION__);
        x11_output_destroy(output);
        return NULL;
    }

    output->conn_destroy_listener.notify = handle_connection_destroy;
    wl_signal_add(&connection->destroy_signal, &output->conn_destroy_listener);

    return base;
}
