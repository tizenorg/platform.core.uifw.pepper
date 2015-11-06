#include "pepper-internal.h"

static void
subcompositor_destroy(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static void
subcompositor_get_subsurface(struct wl_client   *client,
                             struct wl_resource *resource,
                             uint32_t            id,
                             struct wl_resource *surface_resource,
                             struct wl_resource *parent_resource)
{
    /* TODO */
}

static const struct wl_subcompositor_interface subcompositor_interface =
{
    subcompositor_destroy,
    subcompositor_get_subsurface,
};

static void
unbind_resource(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
subcompositor_bind(struct wl_client *client,
                   void             *data,
                   uint32_t          version,
                   uint32_t          id)
{
    pepper_subcompositor_t  *subcompositor = (pepper_subcompositor_t *)data;
    struct wl_resource      *resource;

    resource = wl_resource_create(client, &wl_subcompositor_interface, version, id);

    if (!resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&subcompositor->resource_list, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, &subcompositor_interface, subcompositor, unbind_resource);
}

pepper_subcompositor_t *
pepper_subcompositor_create(pepper_compositor_t *compositor)
{
    pepper_subcompositor_t *subcompositor;

    subcompositor = (pepper_subcompositor_t *)pepper_object_alloc(PEPPER_OBJECT_SUBCOMPOSITOR,
                                                                  sizeof(pepper_subcompositor_t));
    PEPPER_CHECK(subcompositor, goto error, "pepper_object_alloc() failed.\n");

    subcompositor->compositor = compositor;
    subcompositor->global = wl_global_create(compositor->display, &wl_subcompositor_interface, 1,
                                             subcompositor, subcompositor_bind);
    PEPPER_CHECK(subcompositor->global, goto error, "wl_global_create() failed.\n");

    wl_list_init(&subcompositor->resource_list);

    return subcompositor;

error:
    if (subcompositor)
        pepper_subcompositor_destroy(subcompositor);

    return NULL;
}

void
pepper_subcompositor_destroy(pepper_subcompositor_t *subcompositor)
{
    if (subcompositor->global)
        wl_global_destroy(subcompositor->global);

    /* TODO */

    pepper_object_fini(&subcompositor->base);
    free(subcompositor);
}
