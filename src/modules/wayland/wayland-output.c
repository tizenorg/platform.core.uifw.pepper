#include "wayland-internal.h"

static const char *maker_name = "wayland";
static const char *model_name = "wayland";

static void
shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void
shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges,
                        int32_t w, int32_t h)
{
    wayland_output_t *output = data;

    PEPPER_IGNORE(shell_surface);
    PEPPER_IGNORE(edges);

    output->w = w;
    output->h = h;

    wl_signal_emit(&output->mode_change_signal, NULL);
}

static void
shell_surface_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener =
{
    shell_surface_ping,
    shell_surface_configure,
    shell_surface_popup_done,
};

static void
wayland_output_destroy(void *o)
{
    wayland_output_t *output = o;

    wl_signal_emit(&output->destroy_signal, output);

    wl_list_remove(&conn_destroy_listener.link);

    wl_surface_destroy(output->surface);
    wl_shell_surface_destroy(output->shell_surface);

    pepper_free(output);
}

static void
wayland_output_add_destroy_listener(void *o, struct wl_listener *listener)
{
    wayland_output_t *output = o;
    wl_signal_add(&output->destroy_signal, listener);
}

static void
wayland_output_add_mode_change_listener(void *o, struct wl_listener *listener)
{
    wayland_output_t *output = o;
    wl_signal_add(&output->mode_change_signal, listener);
}

static int32_t
wayland_output_get_subpixel_order(void *o)
{
    wayland_output_t *output = o;
    return output->subpixel;
}

static const char *
wayland_output_get_maker_name(void *o)
{
    PEPPER_IGNORE(o);
    return maker_name;
}

static const char *
wayland_output_get_model_name(void *o)
{
    PEPPER_IGNORE(o);
    return model_name;
}

static int
wayland_output_get_mode_count(void *o)
{
    PEPPER_IGNORE(o);

    /* There's only one available mode in wayland backend which is also the current mode. */
    return 1;
}

static void
wayland_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    wayland_output_t *output = o;

    if (index != 0)
        return;

    mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    mode->w = output->w;
    mode->h = output->h;
    mode->refresh = 60000;
}

static pepper_bool_t
wayland_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    wayland_output_t *output = o;

    if (mode->w <= 0 || mode->h <= 0)
        return PEPPER_FALSE;

    if (mode->refresh != 60000)
        return PEPPER_FALSE;

    if (output->w != w || output->h != h)
    {
        output->w = w;
        output->h = h;

        /* TODO: Handle resize here. */

        wl_signal_emit(&output->mode_change_signal, output);
    }

    return PEPPER_TRUE;
}

static const pepper_output_interface_t wayland_output_interface =
{
    wayland_output_destroy,
    wayland_output_add_destroy_listener,
    wayland_output_add_mode_change_listener,

    wayland_output_get_subpixel_order,
    wayland_output_get_maker_name,
    wayland_output_get_model_name,

    wayland_output_get_mode_count,
    wayland_output_get_mode,
    wayland_output_set_mode,
};

PEPPER_API pepper_bool_t
pepper_wayland_output_create(pepper_wayland_t *conn, int32_t w, int32_t h)
{
    wayland_output_t *output;

    output = pepper_calloc(1, sizeof(wayland_output_t));
    if (!output)
        return PEPPER_FALSE;

    output->conn = conn;

    output->w = w;
    output->h = h;

    /* Hard-Coded: subpixel order to horizontal RGB. */
    output->subpixel = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;

    /* Create wayland resources. */
    output->surface = wl_compositor_create_surface(conn->compositor);
    output->shell_surface = wl_shell_get_shell_surface(conn->shell, output->surface);
    wl_shell_surface_add_listener(output->shell_surface, &shell_surface_listener, output);

    /* Add compositor base class output object for this output. */
    if (!pepper_compositor_add_output(conn->compositor, &wayland_output_interface, output))
    {
        wayland_output_destroy(output);
        return PEPPER_FALSE;
    }

    return PEPPER_TRUE;
}
