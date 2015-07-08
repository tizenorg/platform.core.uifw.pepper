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

    output->mode_count = output->backend->get_mode_count(output->data);
    PEPPER_ASSERT(output->mode_count > 0);

    output->modes = pepper_calloc(output->mode_count, sizeof(pepper_output_mode_t));
    if (!output->modes)
    {
        pepper_output_destroy(&output->base);
        return;
    }

    for (i = 0; i < output->mode_count; i++)
    {
        output->backend->get_mode(output->data, i, &output->modes[i]);

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
    output->backend = NULL;
    pepper_output_destroy(&output->base);
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

void
pepper_output_schedule_repaint(pepper_output_t *output)
{
    struct wl_event_loop   *loop;

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

    pepper_compositor_update_views(output->compositor);

    output->backend->repaint(output->data, &output->compositor->view_list, &output->damage_region);
    output->frame.pending = PEPPER_TRUE;
    output->frame.scheduled = PEPPER_FALSE;

    /* TODO: Send frame done to the callback objects of this output. */

    /* Output has been repainted, so damage region is totally consumed. */
    pixman_region32_clear(&output->damage_region);
}

PEPPER_API pepper_object_t *
pepper_compositor_add_output(pepper_object_t *cmp,
                             const pepper_output_backend_t *backend, void *data)
{
    pepper_output_t        *output;
    pepper_compositor_t    *compositor = (pepper_compositor_t *)cmp;
    uint32_t                id;

    CHECK_MAGIC_AND_NON_NULL(cmp, PEPPER_COMPOSITOR);

    id = ffs(~compositor->output_id_allocator);

    if (id == 0)
    {
        PEPPER_ERROR("No available output ids.\n");
        return NULL;
    }

    id = id - 1;

    output = (pepper_output_t *)pepper_object_alloc(sizeof(pepper_output_t), PEPPER_OUTPUT);
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

    output->id = id;
    compositor->output_id_allocator |= (1 << output->id);

    /* Create backend-side object. */
    output->backend = (pepper_output_backend_t *)backend;
    output->data = data;

    /* Initialize output modes. */
    output_update_mode(output);

    /* TODO: Set scale value according to the config or something. */
    output->scale = 1;

    /* Initialize geometry. TODO: Calculate position and size of the output. */
    output->geometry.transform = WL_OUTPUT_TRANSFORM_NORMAL;
    output->geometry.subpixel = backend->get_subpixel_order(data);
    output->geometry.maker = backend->get_maker_name(data);
    output->geometry.model = backend->get_model_name(data);
    output->geometry.x = 0;
    output->geometry.y = 0;
    output->geometry.w = output->current_mode->w;
    output->geometry.h = output->current_mode->h;

    wl_list_insert(&compositor->output_list, &output->link);

    /* Install listeners. */
    output->data_destroy_listener.notify = handle_output_data_destroy;
    backend->add_destroy_listener(data, &output->data_destroy_listener);

    output->mode_change_listener.notify = handle_mode_change;
    backend->add_mode_change_listener(data, &output->mode_change_listener);

    output->frame.frame_listener.notify = handle_output_frame;
    backend->add_frame_listener(data, &output->frame.frame_listener);

    pepper_output_add_damage_whole(&output->base);
    return &output->base;
}

PEPPER_API pepper_object_t *
pepper_output_get_compositor(pepper_object_t *out)
{
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);
    return &(((pepper_output_t *)out)->compositor->base);
}

PEPPER_API void
pepper_output_destroy(pepper_object_t *out)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);

    pepper_object_fini(&output->base);

    output->compositor->output_id_allocator &= ~(1 << output->id);
    wl_list_remove(&output->link);

    if (output->backend && output->data)
        output->backend->destroy(output->data);

    wl_global_destroy(output->global);
    wl_list_remove(&output->data_destroy_listener.link);
    wl_list_remove(&output->mode_change_listener.link);
    wl_list_remove(&output->frame.frame_listener.link);

    /* TODO: Handle removal of this output. e.g. Re-position outputs. */

    pepper_free(output);
}

PEPPER_API void
pepper_output_move(pepper_object_t *out, int32_t x, int32_t y)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);

    if ((output->geometry.x != x) || (output->geometry.y != y))
    {
        output->geometry.x = x;
        output->geometry.y = y;

        pepper_output_add_damage_whole(out);
        output_send_geometry(output);
    }
}

PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_object_t *out)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);
    return &output->geometry;
}

PEPPER_API uint32_t
pepper_output_get_scale(pepper_object_t *out)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);
    return output->scale;
}

PEPPER_API int
pepper_output_get_mode_count(pepper_object_t *out)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);
    return output->mode_count;
}

PEPPER_API const pepper_output_mode_t *
pepper_output_get_mode(pepper_object_t *out, int index)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);

    if (index < 0 || index >= output->mode_count)
        return NULL;

    return &output->modes[index];
}

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_object_t *out, const pepper_output_mode_t *mode)
{
    pepper_output_t *output = (pepper_output_t *)out;
    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);

    if (output->current_mode == mode)
        return PEPPER_TRUE;

    if (output->backend->set_mode(output->data, mode))
    {
        pepper_output_add_damage_whole(out);
        return PEPPER_TRUE;
    }

    return PEPPER_FALSE;
}

PEPPER_API void
pepper_output_add_damage(pepper_object_t *out,
                         const pixman_region32_t *region, int x, int y)
{
    pepper_output_t    *output = (pepper_output_t *)out;
    pixman_region32_t   damage;

    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);

    pixman_region32_init(&damage);
    pixman_region32_copy(&damage, (pixman_region32_t *)region);
    pixman_region32_translate(&damage, x, y);
    pixman_region32_intersect_rect(&damage, &damage, 0, 0, output->geometry.w, output->geometry.h);

    if (pixman_region32_not_empty(&damage))
    {
        pixman_region32_union(&output->damage_region, &output->damage_region, &damage);
        pepper_output_schedule_repaint(output);
    }

    pixman_region32_fini(&damage);
}

PEPPER_API void
pepper_output_add_damage_rect(pepper_object_t *out, int x, int y, unsigned int w, unsigned int h)
{
    pepper_output_t    *output = (pepper_output_t *)out;
    pixman_region32_t   damage;

    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);

    pixman_region32_init_rect(&damage, x, y, w, h);
    pixman_region32_intersect_rect(&damage, &damage, 0, 0, output->geometry.w, output->geometry.h);

    if (pixman_region32_not_empty(&damage))
        pixman_region32_union(&output->damage_region, &output->damage_region, &damage);

    pixman_region32_fini(&damage);
    pepper_output_schedule_repaint(output);
}

PEPPER_API void
pepper_output_add_damage_whole(pepper_object_t *out)
{
    pepper_output_t *output = (pepper_output_t *)out;

    CHECK_MAGIC_AND_NON_NULL(out, PEPPER_OUTPUT);
    pixman_region32_init_rect(&output->damage_region, 0, 0, output->geometry.w, output->geometry.h);
    pepper_output_schedule_repaint(output);
}
