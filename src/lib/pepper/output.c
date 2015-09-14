#include "pepper-internal.h"

static void
output_send_modes(pepper_output_t *output, struct wl_resource *resource)
{
    int i;
    int mode_count = output->backend->get_mode_count(output->data);

    for (i = 0; i < mode_count; i++)
    {
        pepper_output_mode_t mode;

        output->backend->get_mode(output->data, i, &mode);
        wl_output_send_mode(resource, mode.flags, mode.w, mode.h, mode.refresh);
    }

    wl_output_send_done(resource);
}

static void
output_update_mode(pepper_output_t *output)
{
    struct wl_resource *resource;
    int                 i;
    int                 mode_count = output->backend->get_mode_count(output->data);

    for (i = 0; i < mode_count; i++)
    {
        pepper_output_mode_t mode;

        output->backend->get_mode(output->data, i, &mode);

        if (mode.flags & WL_OUTPUT_MODE_CURRENT)
        {
            output->current_mode = mode;

            wl_resource_for_each(resource, &output->resource_list)
            {
                wl_output_send_mode(resource, mode.flags, mode.w, mode.h, mode.refresh);
                wl_output_send_done(resource);
            }
        }
    }
}

static void
output_send_geometry(pepper_output_t *output)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &output->resource_list)
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

    resource = wl_resource_create(client, &wl_output_interface, 2, id);
    if (resource == NULL)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&output->resource_list, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, NULL, NULL, output_destroy);

    wl_output_send_geometry(resource,
                            output->geometry.x, output->geometry.y,
                            output->geometry.w, output->geometry.h,
                            output->geometry.subpixel,
                            output->geometry.maker, output->geometry.model,
                            output->geometry.transform);

    wl_output_send_scale(resource, output->scale);
    output_send_modes(output, resource);
}

static void
output_accumulate_damage(pepper_output_t *output)
{
    pepper_plane_t     *plane;
    pixman_region32_t   clip;
    pixman_region32_t   plane_clip;

    pixman_region32_init(&clip);

    pepper_list_for_each_reverse(plane, &output->plane_list, link)
    {
        pepper_plane_accumulate_damage(plane, &plane_clip);
        pixman_region32_copy(&plane->clip_region, &clip);
        pixman_region32_union(&clip, &clip, &plane_clip);
    }

    pixman_region32_fini(&clip);
}

static void
output_repaint(pepper_output_t *output)
{
    pepper_view_t  *view;
    pepper_plane_t *plane;

    pepper_list_for_each(view, &output->compositor->view_list, compositor_link)
        pepper_view_update(view);

    pepper_list_init(&output->view_list);

    /* Build a list of views in sorted z-order that are visible on the given output. */
    pepper_list_for_each(view, &output->compositor->view_list, compositor_link)
    {
        if (!view->active || !(view->output_overlap & (1 << output->id)) || !view->surface)
        {
            /* Detach from the previously assigned plane. */
            pepper_view_assign_plane(view, output, NULL);
            continue;
        }

        pepper_list_insert(&output->view_list, &view->link);
    }

    output->backend->assign_planes(output->data, &output->view_list);

    pepper_list_for_each(plane, &output->plane_list, link)
        pepper_plane_update(plane, &output->view_list);

    output_accumulate_damage(output);
    output->backend->repaint(output->data, &output->plane_list);

    output->frame.pending = PEPPER_TRUE;
    output->frame.scheduled = PEPPER_FALSE;

    pepper_list_for_each(view, &output->view_list, link)
    {
        /* TODO: Output time stamp and presentation feedback. */
        pepper_surface_send_frame_callback_done(view->surface,
                                                output->frame.time.tv_sec * 1000 +
                                                output->frame.time.tv_nsec / 1000000);
    }
}

static void
idle_repaint(void *data)
{
    pepper_output_t *output = data;

    if (!output->frame.pending)
    {
        /* We can repaint a frame immediately if it is not in pending state. */
        output_repaint(output);
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

PEPPER_API void
pepper_output_add_damage_region(pepper_output_t *output, pixman_region32_t *region)
{
    pepper_plane_t *plane;
    pepper_list_for_each(plane, &output->plane_list, link)
        pepper_plane_add_damage_region(plane, region);
}

PEPPER_API void
pepper_output_finish_frame(pepper_output_t *output, struct timespec *ts)
{
    output->frame.pending = PEPPER_FALSE;

    if (ts)
        output->frame.time = *ts;
    else
        pepper_compositor_get_time(output->compositor, &output->frame.time);

    /* TODO: Better repaint scheduling by putting a delay before repaint. */
    if (output->frame.scheduled)
        output_repaint(output);
}

PEPPER_API void
pepper_output_remove(pepper_output_t *output)
{
    pepper_output_destroy(output);
}

PEPPER_API void
pepper_output_update_mode(pepper_output_t *output)
{
    output_update_mode(output);
    pepper_object_emit_event(&output->base, PEPPER_EVENT_OUTPUT_MODE_CHANGE, NULL);
}

PEPPER_API pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t *compositor,
                             const pepper_output_backend_t *backend, const char *name, void *data)
{
    pepper_output_t    *output;
    uint32_t            id;

    PEPPER_CHECK(name, return NULL, "Output name must be given.\n");

    pepper_list_for_each(output, &compositor->output_list, link)
    {
        PEPPER_CHECK(strcmp(output->name, name) != 0, return NULL,
                     "Output with name = %s already exist.\n", name);
    }

    id = ffs(~compositor->output_id_allocator);
    PEPPER_CHECK(id != 0, return NULL, "No available output ids.\n");

    id = id - 1;

    output = (pepper_output_t *)pepper_object_alloc(PEPPER_OBJECT_OUTPUT, sizeof(pepper_output_t));
    PEPPER_CHECK(output, return NULL, "pepper_object_alloc() failed.\n");

    output->compositor = compositor;
    output->link.item = output;
    wl_list_init(&output->resource_list);

    /* Create global object for this output. */
    output->global = wl_global_create(compositor->display, &wl_output_interface, 2, output,
                                      output_bind);
    if (!output->global)
    {
        free(output);
        return NULL;
    }

    output->id = id;
    compositor->output_id_allocator |= (1 << output->id);
    output->name = strdup(name);

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
    output->geometry.w = output->current_mode.w;
    output->geometry.h = output->current_mode.h;

    pepper_list_insert(&compositor->output_list, &output->link);

    pepper_list_init(&output->plane_list);
    pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_OUTPUT_ADD, output);

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
    pepper_object_emit_event(&output->compositor->base,
                             PEPPER_EVENT_COMPOSITOR_OUTPUT_REMOVE, output);
    pepper_object_fini(&output->base);

    output->compositor->output_id_allocator &= ~(1 << output->id);
    pepper_list_remove(&output->link);
    output->backend->destroy(output->data);
    wl_global_destroy(output->global);

    free(output->name);
    free(output);
}

PEPPER_API void
pepper_output_move(pepper_output_t *output, int32_t x, int32_t y)
{
    if ((output->geometry.x != x) || (output->geometry.y != y))
    {
        output->geometry.x = x;
        output->geometry.y = y;

        /* TODO: pepper_output_add_damage_whole(out); */

        output_send_geometry(output);
        pepper_object_emit_event(&output->base, PEPPER_EVENT_OUTPUT_MOVE, NULL);
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
    return output->backend->get_mode_count(output->data);
}

PEPPER_API void
pepper_output_get_mode(pepper_output_t *output, int index, pepper_output_mode_t *mode)
{
    return output->backend->get_mode(output->data, index, mode);
}

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_output_t *output, const pepper_output_mode_t *mode)
{
    if (output->current_mode.w == mode->w && output->current_mode.h == mode->h &&
        output->current_mode.refresh == mode->refresh)
        return PEPPER_TRUE;

    if (output->backend->set_mode(output->data, mode))
    {
        /* TODO: pepper_output_add_damage_whole(out); */
        return PEPPER_TRUE;
    }

    return PEPPER_FALSE;
}

PEPPER_API const char *
pepper_output_get_name(pepper_output_t *output)
{
    return output->name;
}

PEPPER_API pepper_output_t *
pepper_compositor_find_output(pepper_compositor_t *compositor, const char *name)
{
    pepper_output_t *output;

    pepper_list_for_each(output, &compositor->output_list, link)
    {
        if (strcmp(output->name, name) == 0)
            return output;
    }

    return NULL;
}
