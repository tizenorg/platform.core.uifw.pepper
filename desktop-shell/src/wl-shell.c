#include "desktop-shell-internal.h"

static void
shell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
    /* TODO */
}

static void
shell_surface_move(struct wl_client *client, struct wl_resource *resource,
                   struct wl_resource *seat, uint32_t serial)
{
    /* TODO */
}

static void
shell_surface_resize(struct wl_client *client, struct wl_resource *resource,
                     struct wl_resource *seat, uint32_t serial, uint32_t edges)
{
    /* TODO */
}

static void
shell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource)
{
    /* TODO */
}

static void
shell_surface_set_transient(struct wl_client *client, struct wl_resource *resource,
                            struct wl_resource *parent, int32_t x, int32_t y, uint32_t flags)
{
    /* TODO */
}

static void
shell_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource,
                             uint32_t method, uint32_t framerate, struct wl_resource *output)
{
    /* TODO */
}

static void
shell_surface_set_popup(struct wl_client *client, struct wl_resource *resource,
                        struct wl_resource *seat, uint32_t serial, struct wl_resource *parent,
                        int32_t x, int32_t y, uint32_t flags)
{
    /* TODO */
}

static void
shell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource,
                            struct wl_resource *output)
{
    /* TODO */
}

static void
shell_surface_set_title(struct wl_client *client, struct wl_resource *resource,
                        const char *title)
{
    /* TODO */
}

static void
shell_surface_set_class(struct wl_client *client, struct wl_resource *resource,
                        const char *class_)
{
    /* TODO */
}

static const struct wl_shell_surface_interface shell_surface_implementation =
{
    shell_surface_pong,
    shell_surface_move,
    shell_surface_resize,
    shell_surface_set_toplevel,
    shell_surface_set_transient,
    shell_surface_set_fullscreen,
    shell_surface_set_popup,
    shell_surface_set_maximized,
    shell_surface_set_title,
    shell_surface_set_class,
};

static void
shell_get_shell_surface(struct wl_client *client, struct wl_resource *resource,
                        uint32_t id, struct wl_resource *surface_resource)
{
    pepper_object_t    *surface = wl_resource_get_user_data(surface_resource);
    shell_t            *shell = wl_resource_get_user_data(resource);

    shell_surface_create(shell, surface, client, "wl_shell_surface", &wl_shell_surface_interface,
                         &shell_surface_implementation, 1, id);
}

static const struct wl_shell_interface shell_implementation =
{
    shell_get_shell_surface,
};

static void
bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    shell_create((pepper_object_t *)data, client,
                 &wl_shell_interface, &shell_implementation, version, id);
}

pepper_bool_t
init_wl_shell(pepper_object_t *compositor)
{
    struct wl_display  *display = pepper_compositor_get_display(compositor);
    struct wl_global   *global;

    global = wl_global_create(display, &wl_shell_interface, 1, compositor, bind_shell);
    if (!global)
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}
