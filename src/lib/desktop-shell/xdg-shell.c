#include "desktop-shell-internal.h"
#include "xdg-shell-server-protocol.h"

static void
xdg_surface_destroy(struct wl_client    *client,
                    struct wl_resource  *resource)
{
    wl_resource_destroy(resource);
}

static void
xdg_surface_set_parent(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *parent_resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    shell_surface_t *parent = NULL;

    if (parent_resource)
    {
        parent = wl_resource_get_user_data(resource);
        shell_surface_set_parent(shsurf, parent->surface);
    }
    else
    {
        shell_surface_set_parent(shsurf, NULL);
    }
}

static void
xdg_surface_set_app_id(struct wl_client     *client,
                       struct wl_resource   *resource,
                       const char           *app_id)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_class(shsurf, app_id);
}

static void
xdg_surface_show_window_menu(struct wl_client   *client,
                             struct wl_resource *surface_resource,
                             struct wl_resource *seat_resource,
                             uint32_t            serial,
                             int32_t             x,
                             int32_t             y)
{
    /* TODO: */
}

static void
xdg_surface_set_title(struct wl_client      *client,
                      struct wl_resource    *resource,
                      const char            *title)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_title(shsurf, title);
}

static void
xdg_surface_move(struct wl_client   *client,
                 struct wl_resource *resource,
                 struct wl_resource *seat_resource,
                 uint32_t            serial)
{
    /* TODO: */
}

static void
xdg_surface_resize(struct wl_client     *client,
                   struct wl_resource   *resource,
                   struct wl_resource   *seat_resource,
                   uint32_t              serial,
                   uint32_t              edges)
{
    /* TODO: */
}

static void
xdg_surface_ack_configure(struct wl_client      *client,
                          struct wl_resource    *resource,
                          uint32_t               serial)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_ack_configure(shsurf, serial);
}

static void
xdg_surface_set_window_geometry(struct wl_client    *client,
                                struct wl_resource  *resource,
                                int32_t              x,
                                int32_t              y,
                                int32_t              width,
                                int32_t              height)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_geometry(shsurf, x, y, width, height);
}

static void
xdg_surface_set_maximized(struct wl_client      *client,
                          struct wl_resource    *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_maximized(shsurf, NULL);
}

static void
xdg_surface_unset_maximized(struct wl_client    *client,
                            struct wl_resource  *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_unset_maximized(shsurf);
}

static void
xdg_surface_set_fullscreen(struct wl_client     *client,
                           struct wl_resource   *resource,
                           struct wl_resource   *output_resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    pepper_output_t *output = NULL;

    if (output_resource)
        output = wl_resource_get_user_data(output_resource);

    shell_surface_set_fullscreen(shsurf, output, 0, 0);
}

static void
xdg_surface_unset_fullscreen(struct wl_client   *client,
                             struct wl_resource *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_unset_fullscreen(shsurf);
}

static void
xdg_surface_set_minimized(struct wl_client      *client,
                          struct wl_resource    *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shell_surface_set_minimized(shsurf);
}

static const struct xdg_surface_interface xdg_surface_implementation =
{
    xdg_surface_destroy,
    xdg_surface_set_parent,
    xdg_surface_set_title,
    xdg_surface_set_app_id,
    xdg_surface_show_window_menu,
    xdg_surface_move,
    xdg_surface_resize,
    xdg_surface_ack_configure,
    xdg_surface_set_window_geometry,
    xdg_surface_set_maximized,
    xdg_surface_unset_maximized,
    xdg_surface_set_fullscreen,
    xdg_surface_unset_fullscreen,
    xdg_surface_set_minimized,
};

static void
xdg_shell_destroy(struct wl_client   *client,
                  struct wl_resource *resource)
{
    /* TODO: */
    wl_resource_destroy(resource);
}

static void
xdg_shell_use_unstable_version(struct wl_client     *client,
                               struct wl_resource   *resource,
                               int32_t               version)
{
    /* TODO: */
    if (version != XDG_SHELL_VERSION_CURRENT)
    {
        PEPPER_ERROR("wl_client@%p want bad XDG_SHELL version %d\n", client, version);
    }
}

static void
xdg_shell_get_xdg_surface(struct wl_client      *client,
                          struct wl_resource    *resource,
                          uint32_t               id,
                          struct wl_resource    *surface_resource)
{
    shell_client_t     *shell_client = wl_resource_get_user_data(resource);
    pepper_surface_t   *surface      = wl_resource_get_user_data(surface_resource);
    shell_surface_t    *shsurf;

    if (!pepper_surface_set_role(surface, "xdg_surface"))
    {
        wl_resource_post_error(resource, XDG_SHELL_ERROR_ROLE,
                               "Assign \"xdg_surface\" to wl_surface failed\n");
        return ;
    }

    shsurf = shell_surface_create(shell_client, surface, client, &xdg_surface_interface,
                                  &xdg_surface_implementation, 1, id);
    if (!shsurf)
        wl_client_post_no_memory(client);

    shell_surface_set_toplevel(shsurf);
}

static void
xdg_popup_destroy(struct wl_client *client,
                  struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct xdg_popup_interface xdg_popup_implementation =
{
    xdg_popup_destroy,
};

static void
xdg_shell_get_xdg_popup(struct wl_client    *client,
                        struct wl_resource  *resource,
                        uint32_t             id,
                        struct wl_resource  *surface_resource,
                        struct wl_resource  *parent_resource,
                        struct wl_resource  *seat_resource,
                        uint32_t             serial,
                        int32_t              x,
                        int32_t              y)
{
    shell_client_t      *shell_client = wl_resource_get_user_data(resource);
    pepper_surface_t    *surface;
    pepper_seat_t       *seat;
    pepper_surface_t    *parent;
    shell_surface_t     *shsurf;

    /* Check parameters */
    if (!surface_resource)
    {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "Invalid surface");
        return ;
    }
    surface = wl_resource_get_user_data(surface_resource);

    if (!seat_resource)
    {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "Invalid seat");
        return ;
    }
    seat = wl_resource_get_user_data(seat_resource);

    if (!parent_resource)
    {
        wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "Invalid parent surface");
        return ;
    }
    parent = wl_resource_get_user_data(parent_resource);

    /**
     * TODO: check parent state
     *       1. Parent must have either a xdg_surface or xdg_popup role
     *       2. If there is an existing popup when creating a new popup, the
     *          parent must be the current topmost popup.
     */
    {
        const char *role = pepper_surface_get_role(parent);

        if (strcmp(role, "xdg_surface") && strcmp(role, "xdg_popup"))
        {
            wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
                                   "Invalid xdg_popup parent");
            return ;
        }
    }

    /* Set role */
    if (!pepper_surface_set_role(surface, "xdg_popup"))
    {
        wl_resource_post_error(resource, XDG_SHELL_ERROR_ROLE,
                               "Assign \"xdg_popup\" to wl_surface failed\n");
        return ;
    }

    /* TODO: */

    shsurf = shell_surface_create(shell_client, surface, client, &xdg_popup_interface,
                                  &xdg_popup_implementation, 1, id);
    if (!shsurf)
        wl_client_post_no_memory(client);

    shell_surface_set_popup(shsurf, seat, parent, x, y, 0);
}

static void
xdg_shell_pong(struct wl_client     *client,
               struct wl_resource   *resource,
               uint32_t              serial)
{
    shell_client_t *shell_client = wl_resource_get_user_data(resource);

    shell_client_handle_pong(shell_client, serial);
}

static const struct xdg_shell_interface xdg_shell_implementation =
{
    xdg_shell_destroy,
    xdg_shell_use_unstable_version,
    xdg_shell_get_xdg_surface,
    xdg_shell_get_xdg_popup,
    xdg_shell_pong
};

static void
bind_xdg_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    shell_client_create((desktop_shell_t *)data, client,
                        &xdg_shell_interface, &xdg_shell_implementation, version, id);
}

pepper_bool_t
init_xdg_shell(desktop_shell_t *shell)
{
    struct wl_display  *display = pepper_compositor_get_display(shell->compositor);
    struct wl_global   *global;

    global = wl_global_create(display, &xdg_shell_interface, 1, shell, bind_xdg_shell);
    if (!global)
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

void
fini_xdg_shell(desktop_shell_t *shell)
{
    /* TODO */
}
