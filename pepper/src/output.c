#include "pepper-internal.h"

static void
output_update_mode(pepper_output_t *output)
{
    int                     i;
    struct wl_resource     *resource;
    pepper_output_mode_t   *preferred_mode = NULL;

    output->current_mode = NULL;

    if (output->modes)
        pepper_free(output->modes);

    output->mode_count = output->interface->get_mode_count(output->data);
    PEPPER_ASSERT(output->mode_count > 0);

    output->modes = pepper_calloc(output->mode_count, sizeof(pepper_output_mode_t));
    if (!output->modes)
    {
        pepper_output_destroy(output);
        return;
    }

    for (i = 0; i < output->mode_count; i++)
    {
        output->interface->get_mode(output->data, i, &output->modes[i]);

        if (output->modes[i].flags & WL_OUTPUT_MODE_CURRENT)
            output->current_mode = &output->modes[i];

        if (output->modes[i].flags & WL_OUTPUT_MODE_PREFERRED)
            preferred_mode = &output->modes[i];
    }

    if (!output->current_mode)
        output->current_mode = preferred_mode;

    wl_resource_for_each(resource, &output->resources)
    {
        for (i = 0; i < output->mode_count; i++)
        {
            wl_output_send_mode(resource, output->modes[i].flags,
                                output->modes[i].w, output->modes[i].h,
                                output->modes[i].refresh);
        }

        wl_output_send_done(resource);
    }
}

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
                            output->modes[i].w, output->modes[i].h,
                            output->modes[i].refresh);
    }

    wl_output_send_done(resource);
}

static void
handle_output_data_destroy(struct wl_listener *listener, void *data)
{
    pepper_output_t *output = pepper_container_of(listener, pepper_output_t, data_destroy_listener);
    output->data = NULL;
    output->interface = NULL;
    pepper_output_destroy(output);
}

static void
handle_mode_change(struct wl_listener *listener, void *data)
{
    pepper_output_t *output = pepper_container_of(listener, pepper_output_t, mode_change_listener);
    output_update_mode(output);
}

static void
handle_output_frame(struct wl_listener *listener, void *data)
{
    pepper_output_t *output = pepper_container_of(listener, pepper_output_t, frame.frame_listener);

    output->frame.pending = PEPPER_FALSE;

    /* TODO: Better repaint scheduling by putting a delay before repaint. */
    if (output->frame.scheduled)
        pepper_output_repaint(output);
}

static void
idle_repaint(void *data)
{
    pepper_output_t *output = data;

    if (!output->frame.pending)
    {
        /* We can repaint a frame immediately if it is not in pending state. */
        pepper_output_repaint(output);
    }
}

PEPPER_API void
pepper_output_schedule_repaint(pepper_output_t *output)
{
    struct wl_event_loop *loop;

    if (output->frame.scheduled)
        return;

    /* Schedule on the next idle loop so that it can accumulate surface commits. */
    loop = wl_display_get_event_loop(output->compositor->display);
    wl_event_loop_add_idle(loop, idle_repaint, output);
    output->frame.scheduled = PEPPER_TRUE;
}

void
pepper_output_repaint(pepper_output_t *output)
{
    PEPPER_ASSERT(!output->frame.pending);

    output->interface->repaint(output->data);
    output->frame.pending = PEPPER_TRUE;
    output->frame.scheduled = PEPPER_FALSE;

    /* TODO: Send frame done to the callback objects of this output. */
}

PEPPER_API pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t *compositor,
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

    if (!output->global)
    {
        pepper_free(output);
        return NULL;
    }

    /* Create backend-side object. */
    output->interface = (pepper_output_interface_t *)interface;
    output->data = data;

    /* Initialize output modes. */
    output_update_mode(output);

    /* TODO: Set scale value according to the config or something. */
    output->scale = 1;

    /* Initialize geometry. TODO: Calculate position and size of the output. */
    output->geometry.transform = WL_OUTPUT_TRANSFORM_NORMAL;
    output->geometry.subpixel = interface->get_subpixel_order(data);
    output->geometry.maker = interface->get_maker_name(data);
    output->geometry.model = interface->get_model_name(data);
    output->geometry.x = 0;
    output->geometry.y = 0;
    output->geometry.w = output->current_mode->w;
    output->geometry.h = output->current_mode->h;

    wl_list_insert(&compositor->output_list, &output->link);

    /* Install listeners. */
    output->data_destroy_listener.notify = handle_output_data_destroy;
    interface->add_destroy_listener(data, &output->data_destroy_listener);

    output->mode_change_listener.notify = handle_mode_change;
    interface->add_mode_change_listener(data, &output->mode_change_listener);

    output->frame.frame_listener.notify = handle_output_frame;
    interface->add_frame_listener(data, &output->frame.frame_listener);

    pepper_output_schedule_repaint(output);

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
    wl_list_remove(&output->link);

    if (output->interface && output->data)
        output->interface->destroy(output->data);

    wl_global_destroy(output->global);
    wl_list_remove(&output->data_destroy_listener.link);
    wl_list_remove(&output->mode_change_listener.link);
    wl_list_remove(&output->frame.frame_listener.link);

    /* TODO: Handle removal of this output. e.g. Re-position outputs. */

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
    if (output->current_mode == mode)
        return PEPPER_TRUE;

    return output->interface->set_mode(output->data, mode);
}
