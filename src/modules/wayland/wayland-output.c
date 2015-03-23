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
                        int32_t width, int32_t height)
{
    wayland_output_t *output = data;

    PEPPER_IGNORE(shell_surface);
    PEPPER_IGNORE(edges);

    output->w = width;
    output->h = height;

    pepper_output_update_mode(output->base);
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

static void *
wayland_output_create(pepper_compositor_t *compositor, int32_t w, int32_t h, void *data)
{
    pepper_wayland_output_info_t    *info = data;
    wayland_connection_t            *conn;
    wayland_output_t                *output;

    conn = wayland_get_connection(compositor, info->socket_name);
    if (!conn)
        return NULL;

    output = pepper_calloc(1, sizeof(wayland_output_t));
    if (!output)
        return NULL;

    output->connection = conn;

    output->w = w;
    output->h = h;

    /* Hard-Coded: subpixel order to horizontal RGB. */
    output->subpixel = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;

    /* Hard-Coded: scale value to 1. */
    output->scale = 1;

    output->surface = wl_compositor_create_surface(conn->compositor);
    output->shell_surface = wl_shell_get_shell_surface(conn->shell, output->surface);
    wl_shell_surface_add_listener(output->shell_surface, &shell_surface_listener, output);

    return output;
}

static void
wayland_output_destroy(void *o)
{
    wayland_output_t *output = o;

    wl_surface_destroy(output->surface);
    wl_shell_surface_destroy(output->shell_surface);

    pepper_free(output);
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

static int32_t
wayland_output_get_scale(void *o)
{
    wayland_output_t *output = o;
    return output->scale;
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
    mode->width = output->w;
    mode->height = output->h;
    mode->refresh = 60000;
}

static const pepper_output_interface_t wayland_output_interface =
{
    wayland_output_create,
    wayland_output_destroy,
    wayland_output_get_subpixel_order,
    wayland_output_get_maker_name,
    wayland_output_get_model_name,
    wayland_output_get_scale,
    wayland_output_get_mode_count,
    wayland_output_get_mode,
};

PEPPER_API const pepper_output_interface_t *
pepper_wayland_get_output_interface()
{
    return &wayland_output_interface;
}
