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

PEPPER_API pepper_object_t *
pepper_compositor_create(const char *socket_name)
{
    pepper_compositor_t *compositor = NULL;

    compositor = (pepper_compositor_t *)pepper_object_alloc(sizeof(pepper_compositor_t),
                                                            PEPPER_COMPOSITOR);
    if (!compositor)
        return NULL;

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
    pepper_list_init(&compositor->output_list);
    wl_list_init(&compositor->event_hook_chain);
    pepper_list_init(&compositor->view_list);

    /* Install default input event handler */
    if( NULL == pepper_compositor_add_event_hook(&compositor->base,
                                                 pepper_compositor_event_handler,
                                                 compositor))
    {
        PEPPER_ERROR("Failed to install event handler\n");
        goto error;
    }

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

    return &compositor->base;

error:
    if (compositor)
        pepper_compositor_destroy(&compositor->base);

    return NULL;
}

PEPPER_API void
pepper_compositor_destroy(pepper_object_t *cmp)
{
    pepper_compositor_t *compositor = (pepper_compositor_t *)cmp;
    CHECK_MAGIC_AND_NON_NULL(cmp, PEPPER_COMPOSITOR);

    pepper_object_fini(&compositor->base);

    if (compositor->display)
        wl_display_destroy(compositor->display);

    pepper_free(compositor);
}

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_object_t *cmp)
{
    pepper_compositor_t *compositor = (pepper_compositor_t *)cmp;
    CHECK_MAGIC_AND_NON_NULL(cmp, PEPPER_COMPOSITOR);
    return compositor->display;
}
