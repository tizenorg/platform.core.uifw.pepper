#include "pepper-internal.h"

/* compositor interface */
static void
compositor_create_surface(struct wl_client   *client,
                          struct wl_resource *resource,
                          uint32_t            id)
{
    pepper_compositor_t *compositor = wl_resource_get_user_data(resource);
    pepper_surface_create(compositor, client, resource, id);
}

static void
compositor_create_region(struct wl_client   *client,
                         struct wl_resource *resource,
                         uint32_t            id)
{
    pepper_compositor_t *compositor = wl_resource_get_user_data(resource);
    pepper_region_create(compositor, client, resource, id);
}

static const struct wl_compositor_interface compositor_interface =
{
    compositor_create_surface,
    compositor_create_region
};

static void
compositor_bind(struct wl_client *client,
                void             *data,
                uint32_t          version,
                uint32_t          id)
{
    pepper_compositor_t *compositor = (pepper_compositor_t *)data;
    struct wl_resource  *resource;

    resource = wl_resource_create(client, &wl_compositor_interface, version, id);

    if (!resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &compositor_interface, compositor, NULL);
}

PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name)
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
                     compositor_bind);
    wl_list_init(&compositor->surfaces);
    wl_list_init(&compositor->seat_list);

    if (wl_display_init_shm(compositor->display) != 0)
    {
        PEPPER_ERROR("Failed to initialze shm.\n");
        goto error;
    }

    if (!pepper_data_device_manager_init(compositor->display))
    {
        PEPPER_ERROR("Failed to initialze data device manager.\n");
        goto error;
    }

    wl_list_init(&compositor->layers);
    return compositor;

error:
    if (compositor)
        pepper_compositor_destroy(compositor);

    return NULL;
}

PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor)
{
    if (compositor->display)
        wl_display_destroy(compositor->display);

    pepper_free(compositor);
}

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor)
{
    return compositor->display;
}
