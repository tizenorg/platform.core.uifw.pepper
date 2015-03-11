#include "pepper_internal.h"

extern const struct wl_surface_interface surface_implementation;

void
bind_shell(struct wl_client *, void *, uint32_t, uint32_t);

int
load_input_module(pepper_compositor_t *, const char *);

pepper_seat_t *
pepper_seat_create();

void
bind_seat(struct wl_client *, void *, uint32_t, uint32_t);

/* compositor interface */
static void
compositor_create_surface(struct wl_client   *client,
                          struct wl_resource *resource,
                          uint32_t            id)
{
    pepper_compositor_t *compositor = wl_resource_get_user_data(resource);
    pepper_surface_t    *surface;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    surface = (pepper_surface_t *)pepper_calloc(1, sizeof(pepper_surface_t));
    if (!surface)
    {
        PEPPER_ERROR("%s Surface memory allocation failed\n", __FUNCTION__);
        wl_resource_post_no_memory(resource);
    }

    surface->resource = wl_resource_create(client, &wl_surface_interface,
                                           wl_resource_get_version(resource), id);
    if (!surface->resource)
    {
        PEPPER_ERROR("%s wl_resource_create failed\n", __FUNCTION__);
        pepper_free(surface);
        wl_resource_post_no_memory(resource);
        return ;
    }

    wl_resource_set_implementation(surface->resource, &surface_implementation, surface, NULL);
}

static void
compositor_create_region(struct wl_client   *client,
                         struct wl_resource *resource,
                         uint32_t            id)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static const struct wl_compositor_interface compositor_interface =
{
    compositor_create_surface,
    compositor_create_region
};

static void
bind_compositor(struct wl_client *client,
                void             *data,
                uint32_t          version,
                uint32_t          id)
{
    pepper_compositor_t *compositor = (pepper_compositor_t *)data;
    struct wl_resource  *resource;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    resource = wl_resource_create(client, &wl_compositor_interface, version, id);
    if (!resource)
    {
        PEPPER_ERROR("%s wl_resource_create failed\n", __FUNCTION__);

        wl_client_post_no_memory(client);
        return;
    }
    PEPPER_TRACE("%s wl_resource_create success\n", __FUNCTION__);

    wl_resource_set_implementation(resource, &compositor_interface, compositor, NULL);
}

pepper_compositor_t *
pepper_compositor_create(const char *socket_name,
                         const char *backend_name,
                         const char *input_name,
                         const char *shell_name,
                         const char *renderer_name)
{
    pepper_compositor_t *compositor = NULL;

    compositor = (pepper_compositor_t *)pepper_calloc(1, sizeof (pepper_compositor_t));

    if (!compositor)
        goto error;

    compositor->display = wl_display_create();

    if (!compositor->display)
    {
        PEPPER_ERROR("Failed to create wayland display object.\n");
        goto error;
    }

    if (wl_display_add_socket(compositor->display, socket_name) != 0)
    {
        PEPPER_ERROR("Failed to add socket display = %p socket_name = %s\n",
                     compositor->display, socket_name);
        goto error;
    }

    wl_global_create(compositor->display, &wl_compositor_interface, 3, compositor,
                     bind_compositor);

    /* TODO: Load modules. */

    /* input */
    load_input_module(compositor, input_name);
    compositor->seat = pepper_seat_create();
    wl_global_create(compositor->display, &wl_seat_interface, 4, compositor->seat, bind_seat);

    wl_global_create(compositor->display, &wl_shell_interface, 1, compositor,
                     bind_shell);

    wl_display_init_shm(compositor->display);

    return compositor;

error:
    if (compositor)
        pepper_compositor_destroy(compositor);

    return NULL;
}

void
pepper_compositor_destroy(pepper_compositor_t *compositor)
{
    if (compositor->display)
        wl_display_destroy(compositor->display);

    pepper_free(compositor);
}

void
pepper_compositor_run(pepper_compositor_t *compositor)
{
    wl_display_run(compositor->display);
}

