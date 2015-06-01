#include "pepper-internal.h"

static void
region_resource_destroy_handler(struct wl_resource *resource)
{
    pepper_region_t *region = wl_resource_get_user_data(resource);
    pepper_region_destroy(region);
}

static void
region_destroy(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static void
region_add(struct wl_client *client, struct wl_resource *resource,
           int32_t x, int32_t y, int32_t w, int32_t h)
{
    pepper_region_t *region = wl_resource_get_user_data(resource);
    pixman_region32_union_rect(&region->pixman_region, &region->pixman_region,
                               x, y, w, h);
}

static void
region_subtract(struct wl_client *client, struct wl_resource *resource,
                int32_t x, int32_t y, int32_t w, int32_t h)
{
    pepper_region_t    *region = wl_resource_get_user_data(resource);
    pixman_region32_t   rect;

    pixman_region32_init_rect(&rect, x, y, w, h);
    pixman_region32_subtract(&region->pixman_region, &region->pixman_region, &rect);
    pixman_region32_fini(&rect);
}

static const struct wl_region_interface region_implementation =
{
    region_destroy,
    region_add,
    region_subtract,
};

pepper_region_t *
pepper_region_create(pepper_compositor_t   *compositor,
                     struct wl_client      *client,
                     struct wl_resource    *resource,
                     uint32_t               id)
{
    pepper_region_t *region;

    region = (pepper_region_t *)pepper_calloc(1, sizeof(pepper_region_t));

    if (!region)
    {
        PEPPER_ERROR("Surface memory allocation failed\n");
        wl_resource_post_no_memory(resource);
        return NULL;
    }

    region->compositor = compositor;
    region->resource = wl_resource_create(client, &wl_region_interface, 1, id);

    if (!region->resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        pepper_free(region);
        wl_resource_post_no_memory(resource);
        pepper_free(region);
        return NULL;
    }

    wl_resource_set_implementation(region->resource, &region_implementation,
                                   region, region_resource_destroy_handler);
    wl_list_insert(&compositor->regions, wl_resource_get_link(region->resource));

    return region;
}

void
pepper_region_destroy(pepper_region_t *region)
{
    pixman_region32_fini(&region->pixman_region);
}
