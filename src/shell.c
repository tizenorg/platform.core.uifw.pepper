#include "pepper-internal.h"

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
handle_surface_destroy(struct wl_listener *listener, void *data)
{
    pepper_shell_surface_t  *shell_surface = wl_container_of(listener, shell_surface,
                                                             surface_destroy_listener);
    wl_list_remove(&shell_surface->link);
    wl_resource_destroy(shell_surface->resource);
    pepper_free(shell_surface);

    return;
}

/* TODO */
static void
shell_get_shell_surface(struct wl_client *client, struct wl_resource *resource,
                        uint32_t id, struct wl_resource *surface_resource)
{
    pepper_surface_t        *surface = (pepper_surface_t *)
                                        wl_resource_get_user_data(surface_resource);
    pepper_shell_t          *shell = (pepper_shell_t *)wl_resource_get_user_data(resource);
    pepper_shell_surface_t  *shell_surface;
    struct wl_resource      *r;

    shell_surface = (pepper_shell_surface_t *)pepper_calloc(1, sizeof(pepper_shell_surface_t));
    if (!shell_surface)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    r = wl_resource_create(client, &wl_shell_surface_interface, 1/* FIXME */, id);
    if (!r)
    {
        PEPPER_ERROR("Failed to create a wl_resource object in %s\n", __FUNCTION__);
        goto error;
    }

    shell_surface->surface = surface;
    shell_surface->resource = r;
    wl_list_insert(&shell->shell_surfaces, &shell_surface->link);

    wl_resource_set_implementation(r, &shell_surface_implementation, shell_surface, NULL);

    shell_surface->surface_destroy_listener.notify = handle_surface_destroy;
    wl_signal_add(&surface->destroy_signal, &shell_surface->surface_destroy_listener);

    /* TODO */

    return;

error:
    if (shell_surface)
        pepper_free(shell_surface);

    wl_resource_post_no_memory(resource);

    return;
}

static const struct wl_shell_interface shell_implementation =
{
    shell_get_shell_surface,
};

static void
bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    pepper_shell_t      *shell = (pepper_shell_t *)data;
    struct wl_resource  *resource;

    resource = wl_resource_create(client, &wl_shell_interface, 1/* FIXME */, id);
    if (!resource)
    {
        PEPPER_ERROR("Failed to create a wl_resource object in %s\n", __FUNCTION__);
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&shell->resources, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, &shell_implementation, shell, NULL);

    return;
}

pepper_shell_t *
pepper_shell_create(pepper_compositor_t *compositor)
{
    pepper_shell_t  *shell;

    shell = (pepper_shell_t *)pepper_calloc(1, sizeof(pepper_shell_t));
    if (!shell)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    wl_list_init(&shell->resources);
    wl_list_init(&shell->shell_surfaces);

    shell->global = wl_global_create(compositor->display, &wl_shell_interface, 1, shell,
                                     bind_shell);
    if (!shell->global)
    {
        PEPPER_ERROR("Failed to create wl_global in %s\n", __FUNCTION__);
        goto error;
    }

    /* TODO */

    return shell;

error:
    if (shell)
        pepper_free(shell);

    return NULL;
}

void
pepper_shell_destroy(pepper_shell_t *shell)
{
    struct wl_resource  *resource;

    wl_resource_for_each(resource, &shell->resources)
    {
        wl_list_remove(wl_resource_get_link(resource));
        wl_resource_destroy(resource);
    }

    wl_global_destroy(shell->global);
    pepper_free(shell);
    return;
}
