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

void
pepper_compositor_schedule_repaint(pepper_compositor_t *compositor)
{
    pepper_output_t *output;

    pepper_list_for_each(output, &compositor->output_list, link)
        pepper_output_schedule_repaint(output);
}

PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name)
{
    pepper_compositor_t *compositor =
        (pepper_compositor_t *)pepper_object_alloc(PEPPER_OBJECT_COMPOSITOR,
                                                   sizeof(pepper_compositor_t));

    if (!compositor)
    {
        PEPPER_ERROR("pepper_object_alloc() failed.\n");
        goto error;
    }

    compositor->display = wl_display_create();
    if (!compositor->display)
    {
        PEPPER_ERROR("wl_display_create() failed.\n");
        goto error;
    }

    if (wl_display_add_socket(compositor->display, socket_name) != 0)
    {
        PEPPER_ERROR("wl_display_add_socket(%p, %s) failed.\n", compositor->display, socket_name);
        goto error;
    }

    pepper_list_init(&compositor->surface_list);
    pepper_list_init(&compositor->seat_list);
    pepper_list_init(&compositor->output_list);
    pepper_list_init(&compositor->view_list);
    pepper_list_init(&compositor->region_list);

    if (wl_display_init_shm(compositor->display) != 0)
    {
        PEPPER_ERROR("wl_display_init_shm() failed.\n");
        goto error;
    }

    compositor->global = wl_global_create(compositor->display, &wl_compositor_interface,
                                          3, compositor, compositor_bind);
    if (!compositor->global)
    {
        PEPPER_ERROR("wl_global_create() failed.\n");
        goto error;
    }

    if (!pepper_data_device_manager_init(compositor->display))
    {
        PEPPER_ERROR("pepper_data_device_manager_init() failed.\n");
        goto error;
    }

    return compositor;

error:
    if (compositor)
    {
        /* TODO: Data device manager fini. */

        if (compositor->global)
            wl_global_destroy(compositor->global);

        if (compositor->display)
            wl_display_destroy(compositor->display);

        pepper_object_fini(&compositor->base);
        pepper_free(compositor);
    }

    return NULL;
}

PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor)
{
    /* TODO: Data device manager fini. */

    pepper_object_fini(&compositor->base);
    wl_global_destroy(compositor->global);
    wl_display_destroy(compositor->display);
    pepper_free(compositor);
}

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor)
{
    return compositor->display;
}

PEPPER_API const pepper_list_t *
pepper_compositor_get_output_list(pepper_compositor_t *compositor)
{
    return &compositor->output_list;
}

PEPPER_API pepper_view_t *
pepper_compositor_pick_view(pepper_compositor_t *compositor,
                            double x, double y, double *vx, double *vy)
{
    pepper_view_t  *view;
    int             ix = (int)x;
    int             iy = (int)y;

    pepper_list_for_each(view, &compositor->view_list, compositor_link)
    {
        double  lx, ly;
        int     ilx, ily;

        pepper_view_update(view);

        if (!pixman_region32_contains_point(&view->bounding_region, ix, iy, NULL))
            continue;

        pepper_view_get_local_coordinate(view, x, y, &lx, &ly);

        ilx = (int)lx;
        ily = (int)ly;

        if (ilx < 0 || ily < 0 || ilx >= view->w || ily >= view->h)
            continue;

        if (!pixman_region32_contains_point(&view->surface->input_region, ilx, ily, NULL))
            continue;

        if (vx)
            *vx = lx;

        if (vy)
            *vy = ly;

        return view;
    }

    return NULL;
}
