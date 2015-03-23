#include "pepper-internal.h"

static void
surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_attach(struct wl_client   *client,
               struct wl_resource *resource,
               struct wl_resource *buffer,
               int32_t x,
               int32_t y)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_damage(struct wl_client   *client,
               struct wl_resource *resource,
               int32_t x,
               int32_t y,
               int32_t width,
               int32_t height)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_frame(struct wl_client   *client,
              struct wl_resource *resource,
              uint32_t           callback)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_set_opaque_region(struct wl_client   *client,
                          struct wl_resource *resource,
                          struct wl_resource *region)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_set_input_region(struct wl_client   *client,
                         struct wl_resource *resource,
                         struct wl_resource *region)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_commit(struct wl_client *client, struct wl_resource *resource)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_set_buffer_transform(struct wl_client   *client,
                             struct wl_resource *resource,
                             int                 transform)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
surface_set_buffer_scale(struct wl_client   *client,
                         struct wl_resource *resource,
                         int32_t             scale)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static const struct wl_surface_interface surface_implementation =
{
    surface_destroy,
    surface_attach,
    surface_damage,
    surface_frame,
    surface_set_opaque_region,
    surface_set_input_region,
    surface_commit,
    surface_set_buffer_transform,
    surface_set_buffer_scale
};

pepper_surface_t *
pepper_surface_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id)
{
    pepper_surface_t *surface;

    surface = (pepper_surface_t *)pepper_calloc(1, sizeof(pepper_surface_t));

    if (!surface)
    {
        PEPPER_ERROR("Surface memory allocation failed\n");
        wl_resource_post_no_memory(resource);
        return NULL;
    }

    surface->resource = wl_resource_create(client, &wl_surface_interface,
                                           wl_resource_get_version(resource), id);

    if (!surface->resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        pepper_free(surface);
        wl_resource_post_no_memory(resource);
        pepper_free(surface);
        return NULL;
    }

    wl_resource_set_implementation(surface->resource, &surface_implementation, surface, NULL);
    wl_list_insert(&compositor->surfaces, wl_resource_get_link(surface->resource));

    return surface;
}
