#include "desktop-shell-internal.h"
#include "xdg-shell-server-protocol.h"

static void
xdg_surface_destroy(struct wl_client    *client,
                    struct wl_resource  *resource)
{
    /* TODO: */
}

static void
xdg_surface_set_parent(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *parent_resource)
{
    /* TODO: */
}

static void
xdg_surface_set_app_id(struct wl_client     *client,
                       struct wl_resource   *resource,
                       const char           *app_id)
{
    /* TODO: */
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
    /* TODO: */
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
    /* TODO: */
}

static void
xdg_surface_set_window_geometry(struct wl_client    *client,
                                struct wl_resource  *resource,
                                int32_t              x,
                                int32_t              y,
                                int32_t              width,
                                int32_t              height)
{
    /* TODO: */
}

static void
xdg_surface_set_maximized(struct wl_client      *client,
                          struct wl_resource    *resource)
{
    /* TODO: */
}

static void
xdg_surface_unset_maximized(struct wl_client    *client,
                            struct wl_resource  *resource)
{
    /* TODO: */
}

static void
xdg_surface_set_fullscreen(struct wl_client     *client,
                           struct wl_resource   *resource,
                           struct wl_resource   *output_resource)
{
    /* TODO: */
}

static void
xdg_surface_unset_fullscreen(struct wl_client   *client,
                             struct wl_resource *resource)
{
    /* TODO: */
}

static void
xdg_surface_set_minimized(struct wl_client      *client,
                          struct wl_resource    *resource)
{
    /* TODO: */
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
}

static void
xdg_shell_use_unstable_version(struct wl_client     *client,
                               struct wl_resource   *resource,
                               int32_t               version)
{
    /* TODO: */
    if (version != XDG_SHELL_VERSION_CURRENT)
        PEPPER_ERROR("wl_client@%p want bad XDG_SHELL version %d\n", client, version);
}

static void
xdg_shell_get_xdg_surface(struct wl_client      *client,
                          struct wl_resource    *resource,
                          uint32_t               id,
                          struct wl_resource    *surface_resource)
{
    shell_client_t     *shell_client = wl_resource_get_user_data(resource);
    pepper_object_t    *surface      = wl_resource_get_user_data(surface_resource);
    shell_surface_t    *shsurf;

    if (!pepper_surface_set_role(surface, "xdg_surface"))
    {
        wl_resource_post_error(resource, XDG_SHELL_ERROR_ROLE,
                               "Assign \"xdg_surface\" to wl_surface failed\n");
        return ;
    }

    /* TODO: */

    shsurf = shell_surface_create(shell_client, surface, client, &xdg_surface_interface,
                                  &xdg_surface_implementation, 1, id);
    if (!shsurf)
        wl_client_post_no_memory(client);
}

static void
xdg_popup_destroy(struct wl_client *client,
                  struct wl_resource *resource)
{
    /* TODO: */
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
    shell_client_t     *shell_client = wl_resource_get_user_data(resource);
    pepper_object_t    *surface      = wl_resource_get_user_data(surface_resource);
    shell_surface_t    *shsurf;

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
