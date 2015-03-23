#include "pepper-internal.h"

static void
output_send_geometry(pepper_output_t *output)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &output->resources)
    {
        wl_output_send_geometry(resource,
                                output->geometry.x, output->geometry.y,
                                output->geometry.w, output->geometry.h,
                                output->geometry.subpixel,
                                output->geometry.maker, output->geometry.model,
                                output->geometry.transform);
    }
}

static void
output_destroy(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
output_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    struct wl_resource      *resource;
    pepper_output_t         *output = (pepper_output_t *)data;
    int                     i;

    resource = wl_resource_create(client, &wl_output_interface, 2, id);

    if (resource == NULL)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&output->resources, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, NULL, NULL, output_destroy);

    wl_output_send_geometry(resource,
                            output->geometry.x, output->geometry.y,
                            output->geometry.w, output->geometry.h,
                            output->geometry.subpixel,
                            output->geometry.maker, output->geometry.model,
                            output->geometry.transform);

    wl_output_send_scale(resource, output->scale);

    for (i = 0; i < output->mode_count; i++)
    {
        wl_output_send_mode(resource, output->modes[i].flags,
                            output->modes[i].width, output->modes[i].height,
                            output->modes[i].refresh);
    }

    wl_output_send_done(resource);
}

PEPPER_API pepper_output_t *
pepper_output_create(pepper_compositor_t *compositor,
                     int32_t x, int32_t y, int32_t w, int32_t h, int32_t transform, int32_t scale,
                     const pepper_output_interface_t *interface, void *data)
{
    pepper_output_t     *output;

    output = pepper_calloc(1, sizeof(pepper_output_t));
    if (!output)
        return NULL;

    wl_list_init(&output->resources);
    output->compositor = compositor;

    /* Create global object for this output. */
    output->global = wl_global_create(compositor->display, &wl_output_interface, 2, output,
                                      output_bind);

    /* Create backend-side object. */
    output->interface = *interface;
    output->data = interface->create(compositor, w, h, data);

    /* Initialize output geometry. */
    output->geometry.x = x;
    output->geometry.y = y;
    output->geometry.w = w;
    output->geometry.h = h;
    output->geometry.transform;
    output->geometry.subpixel = interface->get_subpixel_order(output->data);
    output->geometry.maker = interface->get_maker_name(output->data);
    output->geometry.model = interface->get_model_name(output->data);

    /* Initialize output scale. */
    output->scale = interface->get_scale(output->data);

    /* Initialize output modes. */
    pepper_output_update_mode(output);

    return output;
}

PEPPER_API pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output)
{
    return output->compositor;
}

PEPPER_API void
pepper_output_destroy(pepper_output_t *output)
{
    output->interface.destroy(output->data);
    pepper_free(output);
}

PEPPER_API void
pepper_output_move(pepper_output_t *output, int32_t x, int32_t y)
{
    if ((output->geometry.x != x) || (output->geometry.y != y))
    {
        output->geometry.x = x;
        output->geometry.y = y;

        /* TODO: Repaint. */

        output_send_geometry(output);
    }
}

PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_output_t *output)
{
    return &output->geometry;
}

PEPPER_API uint32_t
pepper_output_get_scale(pepper_output_t *output)
{
    return output->scale;
}

PEPPER_API int
pepper_output_get_mode_count(pepper_output_t *output)
{
    return output->mode_count;
}

PEPPER_API const pepper_output_mode_t *
pepper_output_get_mode(pepper_output_t *output, int index)
{
    if (index < 0 || index >= output->mode_count)
        return NULL;

    return &output->modes[index];
}

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_output_t *output, const pepper_output_mode_t *mode)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
    return PEPPER_FALSE;
}

PEPPER_API void
pepper_output_update_mode(pepper_output_t *output)
{
    int                 i;
    struct wl_resource  *resource;

    output->current_mode = NULL;

    if (output->modes)
        pepper_free(output->modes);

    output->mode_count = output->interface.get_mode_count(output->data);
    PEPPER_ASSERT(output->mode_count > 0);

    output->modes = pepper_calloc(output->mode_count, sizeof(pepper_output_mode_t));
    if (!output->modes)
    {
        pepper_output_destroy(output);
        return;
    }

    for (i = 0; i < output->mode_count; i++)
    {
        output->interface.get_mode(output->data, i, &output->modes[i]);

        if (output->modes[i].flags & WL_OUTPUT_MODE_CURRENT)
            output->current_mode = &output->modes[i];

    }

    wl_resource_for_each(resource, &output->resources)
    {
        for (i = 0; i < output->mode_count; i++)
        {
            wl_output_send_mode(resource, output->modes[i].flags,
                                output->modes[i].width, output->modes[i].height,
                                output->modes[i].refresh);
        }

        wl_output_send_done(resource);
    }
}
