#include "desktop-shell-internal.h"

static void
wl_shell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_handle_pong(shsurf, serial);
}

static void
wl_shell_surface_move(struct wl_client      *client,
                      struct wl_resource    *resource,
                      struct wl_resource    *seat_resource,
                      uint32_t               serial)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    pepper_seat_t   *seat   = wl_resource_get_user_data(seat_resource);

    shell_surface_move(shsurf, seat, serial);
}

static void
wl_shell_surface_resize(struct wl_client *client, struct wl_resource *resource,
                     struct wl_resource *seat, uint32_t serial, uint32_t edges)
{
    /* TODO */
}

static void
wl_shell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_toplevel(shsurf);
}

static void
wl_shell_surface_set_transient(struct wl_client     *client,
                               struct wl_resource   *resource,
                               struct wl_resource   *parent_resource,
                               int32_t               x,
                               int32_t               y,
                               uint32_t              flags)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    pepper_surface_t *parent = wl_resource_get_user_data(parent_resource);

    shell_surface_set_transient(shsurf, parent, x, y, flags);
}

static void
wl_shell_surface_set_fullscreen(struct wl_client    *client,
                                struct wl_resource  *resource,
                                uint32_t             method,
                                uint32_t             framerate,
                                struct wl_resource  *output_resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    pepper_output_t *output = NULL;

    if (output_resource)
        output = wl_resource_get_user_data(output_resource);

    shell_surface_set_fullscreen(shsurf, output, method, framerate);
}

static void
wl_shell_surface_set_popup(struct wl_client     *client,
                           struct wl_resource   *resource,
                           struct wl_resource   *seat_res,
                           uint32_t              serial,
                           struct wl_resource   *parent_res,
                           int32_t               x,
                           int32_t               y,
                           uint32_t              flags)
{
    shell_surface_t     *shsurf = wl_resource_get_user_data(resource);
    pepper_seat_t       *seat;
    pepper_surface_t    *parent;

    if (!seat_res)
    {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "Invalid seat");
        return ;
    }
    seat = wl_resource_get_user_data(seat_res);

    if (!parent_res)
    {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "Invalid parent surface");
        return ;
    }
    parent = wl_resource_get_user_data(parent_res);

    shell_surface_set_popup(shsurf, seat, parent, x, y, flags);
}

static void
wl_shell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource,
                            struct wl_resource *output_res)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    pepper_output_t *output = NULL;

    if (output_res)
        output = wl_resource_get_user_data(output_res);

    shell_surface_set_maximized(shsurf, output);
}

static void
wl_shell_surface_set_title(struct wl_client *client, struct wl_resource *resource,
                        const char *title)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_title(shsurf, title);
}

static void
wl_shell_surface_set_class(struct wl_client *client, struct wl_resource *resource,
                        const char *class_)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_class(shsurf, class_);
}

static const struct wl_shell_surface_interface shell_surface_implementation =
{
    wl_shell_surface_pong,
    wl_shell_surface_move,
    wl_shell_surface_resize,
    wl_shell_surface_set_toplevel,
    wl_shell_surface_set_transient,
    wl_shell_surface_set_fullscreen,
    wl_shell_surface_set_popup,
    wl_shell_surface_set_maximized,
    wl_shell_surface_set_title,
    wl_shell_surface_set_class,
};

static void
wl_shell_get_shell_surface(struct wl_client *client, struct wl_resource *resource,
                           uint32_t id, struct wl_resource *surface_resource)
{
    shell_client_t     *shell_client = wl_resource_get_user_data(resource);
    pepper_surface_t   *surface      = wl_resource_get_user_data(surface_resource);
    shell_surface_t    *shsurf;

    /* Only one shell surface can be associated with a given surface.*/
    if (!pepper_surface_set_role(surface, "wl_shell_surface"))
    {
        wl_resource_post_error(resource, WL_SHELL_ERROR_ROLE,
                               "Assign \"wl_shell_surface\" to wl_surface failed\n");
        return ;
    }

    shsurf = shell_surface_create(shell_client, surface, client, &wl_shell_surface_interface,
                                  &shell_surface_implementation, 1, id);
    if (!shsurf)
        wl_client_post_no_memory(client);
}

static const struct wl_shell_interface shell_implementation =
{
    wl_shell_get_shell_surface,
};

static void
bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    shell_client_create((desktop_shell_t *)data, client,
                        &wl_shell_interface, &shell_implementation, version, id);
}

pepper_bool_t
init_wl_shell(desktop_shell_t *shell)
{
    struct wl_display  *display = pepper_compositor_get_display(shell->compositor);
    struct wl_global   *global;

    global = wl_global_create(display, &wl_shell_interface, 1, shell, bind_shell);
    if (!global)
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

void
fini_wl_shell(desktop_shell_t *shell)
{
    /* TODO */
}
