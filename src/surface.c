#include "pepper_internal.h"
#include "debug_ch.h"

DECLARE_DEBUG_CHANNEL(surface);

void *
pepper_surface_get_buffer(pepper_surface_t *surface)
{
    return surface->buffer;
}

/* surface interface */
static void
surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
    TRACE("enter\n");

    wl_resource_destroy(resource);
}

static void
surface_attach(struct wl_client   *client,
               struct wl_resource *resource,
               struct wl_resource *buffer,
               int32_t x,
               int32_t y)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_damage(struct wl_client   *client,
               struct wl_resource *resource,
               int32_t x,
               int32_t y,
               int32_t width,
               int32_t height)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_frame(struct wl_client   *client,
              struct wl_resource *resource,
              uint32_t           callback)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_set_opaque_region(struct wl_client   *client,
                          struct wl_resource *resource,
                          struct wl_resource *region)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_set_input_region(struct wl_client   *client,
                         struct wl_resource *resource,
                         struct wl_resource *region)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_commit(struct wl_client *client, struct wl_resource *resource)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_set_buffer_transform(struct wl_client   *client,
                             struct wl_resource *resource,
                             int                 transform)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

static void
surface_set_buffer_scale(struct wl_client   *client,
                         struct wl_resource *resource,
                         int32_t             scale)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    TRACE("enter\n");
}

const struct wl_surface_interface surface_implementation =
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
