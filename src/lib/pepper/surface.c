/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "pepper-internal.h"

static void
surface_update_size(pepper_surface_t *surface)
{
    if (surface->buffer.buffer)
    {
        switch (surface->buffer.transform)
        {
        case WL_OUTPUT_TRANSFORM_NORMAL:
        case WL_OUTPUT_TRANSFORM_180:
        case WL_OUTPUT_TRANSFORM_FLIPPED:
        case WL_OUTPUT_TRANSFORM_FLIPPED_180:
            surface->w = surface->buffer.buffer->w;
            surface->h = surface->buffer.buffer->h;
            break;
        case WL_OUTPUT_TRANSFORM_90:
        case WL_OUTPUT_TRANSFORM_270:
        case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        case WL_OUTPUT_TRANSFORM_FLIPPED_270:
            surface->w = surface->buffer.buffer->h;
            surface->h = surface->buffer.buffer->w;
            break;
        }

        surface->w /= surface->buffer.scale;
        surface->h /= surface->buffer.scale;
    }
    else
    {
        surface->w = 0;
        surface->h = 0;
    }
}

static void
surface_state_handle_buffer_destroy(pepper_event_listener_t *listener,
                                    pepper_object_t *object, uint32_t id, void *info, void *data)
{
    pepper_surface_state_t *state = data;
    state->buffer = NULL;
}

static void
surface_handle_buffer_release(pepper_event_listener_t *listener,
                              pepper_object_t *object, uint32_t id, void *info, void *data)
{
    pepper_surface_t *surface = data;
    surface->buffer.buffer = NULL;
    pepper_event_listener_remove(listener);
    pepper_event_listener_remove(surface->buffer.destroy_listener);
}

static void
surface_handle_buffer_destroy(pepper_event_listener_t *listener,
                              pepper_object_t *object, uint32_t id, void *info, void *data)
{
    pepper_surface_t *surface = data;
    surface->buffer.buffer = NULL;
    surface_update_size(surface);
}

void
pepper_surface_state_init(pepper_surface_state_t *state)
{
    state->buffer = NULL;
    state->x = 0;
    state->y = 0;
    state->transform = WL_OUTPUT_TRANSFORM_NORMAL;
    state->scale = 1;

    pixman_region32_init(&state->damage_region);
    pixman_region32_init(&state->opaque_region);
    pixman_region32_init_rect(&state->input_region, INT32_MIN, INT32_MIN, UINT32_MAX, UINT32_MAX);

    wl_list_init(&state->frame_callback_list);
}

void
pepper_surface_state_fini(pepper_surface_state_t *state)
{
    struct wl_resource *callback, *next;

    pixman_region32_fini(&state->damage_region);
    pixman_region32_fini(&state->opaque_region);
    pixman_region32_fini(&state->input_region);

    wl_resource_for_each_safe(callback, next, &state->frame_callback_list)
        wl_resource_destroy(callback);

    if (state->buffer)
        pepper_event_listener_remove(state->buffer_destroy_listener);
}

static void
surface_resource_destroy_handler(struct wl_resource *resource)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);
    pepper_surface_destroy(surface);
}

static void
surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static void
surface_attach(struct wl_client    *client,
               struct wl_resource  *resource,
               struct wl_resource  *buffer_resource,
               int32_t              x,
               int32_t              y)
{
    pepper_surface_t   *surface = wl_resource_get_user_data(resource);
    pepper_buffer_t    *buffer = NULL;

    if (buffer_resource)
    {
        buffer = pepper_buffer_from_resource(buffer_resource);
        if (!buffer)
        {
            wl_client_post_no_memory(client);
            return;
        }
    }

    if (surface->pending.buffer == buffer)
        return;

    if (surface->pending.buffer)
        pepper_event_listener_remove(surface->pending.buffer_destroy_listener);

    surface->pending.buffer = buffer;
    surface->pending.x = x;
    surface->pending.y = y;
    surface->pending.newly_attached = PEPPER_TRUE;

    if (buffer)
    {
        surface->pending.buffer_destroy_listener =
            pepper_object_add_event_listener(&buffer->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                             surface_state_handle_buffer_destroy, &surface->pending);
    }
}

static void
surface_damage(struct wl_client    *client,
               struct wl_resource  *resource,
               int32_t              x,
               int32_t              y,
               int32_t              w,
               int32_t              h)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);
    pixman_region32_union_rect(&surface->pending.damage_region,
                               &surface->pending.damage_region, x, y, w, h);
}

static void
frame_callback_resource_destroy_handler(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
surface_frame(struct wl_client     *client,
              struct wl_resource   *resource,
              uint32_t              callback_id)
{
    pepper_surface_t   *surface = wl_resource_get_user_data(resource);
    struct wl_resource *callback;

    callback = wl_resource_create(client, &wl_callback_interface, 1, callback_id);

    if (!callback)
    {
        wl_resource_post_no_memory(resource);
        return;
    }

    wl_resource_set_implementation(callback, NULL, NULL,
                                   frame_callback_resource_destroy_handler);
    wl_list_insert(surface->pending.frame_callback_list.prev, wl_resource_get_link(callback));
}

static void
surface_set_opaque_region(struct wl_client   *client,
                          struct wl_resource *resource,
                          struct wl_resource *region_resource)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    if (region_resource)
    {
        pepper_region_t *region = wl_resource_get_user_data(region_resource);
        pixman_region32_copy(&surface->pending.opaque_region, &region->pixman_region);
    }
    else
    {
        pixman_region32_clear(&surface->pending.opaque_region);
    }
}

static void
surface_set_input_region(struct wl_client   *client,
                         struct wl_resource *resource,
                         struct wl_resource *region_resource)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    if (region_resource)
    {
        pepper_region_t *region = wl_resource_get_user_data(region_resource);
        pixman_region32_copy(&surface->pending.input_region, &region->pixman_region);
    }
    else
    {
        pixman_region32_init_rect(&surface->pending.input_region,
                                  INT32_MIN, INT32_MIN, UINT32_MAX, UINT32_MAX);
    }
}

static void
surface_commit(struct wl_client *client, struct wl_resource *resource)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);
    pepper_surface_commit(surface);
}

static void
surface_set_buffer_transform(struct wl_client   *client,
                             struct wl_resource *resource,
                             int                 transform)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    if (transform < 0 || transform > WL_OUTPUT_TRANSFORM_FLIPPED_270)
    {
        wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_TRANSFORM,
                               "Invalid transform value : %d", transform);
        return;
    }

    surface->pending.transform = transform;
}

static void
surface_set_buffer_scale(struct wl_client   *client,
                         struct wl_resource *resource,
                         int32_t             scale)
{
    pepper_surface_t *surface = wl_resource_get_user_data(resource);

    if (scale < 1)
    {
        wl_resource_post_error(resource, WL_SURFACE_ERROR_INVALID_SCALE,
                               "Invalid scale value (should be >= 1): %d", scale);
        return;
    }

    surface->pending.scale = scale;
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
    pepper_surface_t *surface = (pepper_surface_t *)pepper_object_alloc(PEPPER_OBJECT_SURFACE,
                                                                        sizeof(pepper_surface_t));
    PEPPER_CHECK(surface, goto error, "pepper_object_alloc() failed.\n");

    surface->compositor = compositor;
    surface->resource = wl_resource_create(client, &wl_surface_interface,
                                           wl_resource_get_version(resource), id);
    PEPPER_CHECK(surface->resource, goto error, "wl_resource_create() failed\n");

    wl_resource_set_implementation(surface->resource, &surface_implementation, surface,
                                   surface_resource_destroy_handler);

    surface->link.item = surface;
    pepper_list_insert(&compositor->surface_list, &surface->link);
    pepper_surface_state_init(&surface->pending);

    surface->buffer.transform = WL_OUTPUT_TRANSFORM_NORMAL;
    surface->buffer.scale = 1;

    pixman_region32_init(&surface->damage_region);
    pixman_region32_init(&surface->opaque_region);
    pixman_region32_init_rect(&surface->input_region, INT32_MIN, INT32_MIN, UINT32_MAX, UINT32_MAX);

    wl_list_init(&surface->frame_callback_list);
    pepper_list_init(&surface->view_list);
    pepper_list_init(&surface->subsurface_list);
    pepper_list_init(&surface->subsurface_pending_list);
    pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_SURFACE_ADD, surface);

    return surface;

error:
    if (surface)
        free(surface);

    return NULL;
}

void
pepper_surface_destroy(pepper_surface_t *surface)
{
    struct wl_resource *callback, *nc;
    pepper_view_t      *view, *nv;

    pepper_object_emit_event(&surface->compositor->base,
                             PEPPER_EVENT_COMPOSITOR_SURFACE_REMOVE, surface);
    pepper_surface_state_fini(&surface->pending);
    pepper_object_fini(&surface->base);

    pepper_list_for_each_safe(view, nv, &surface->view_list, surface_link)
        pepper_view_set_surface(view, NULL);

    if (surface->buffer.buffer)
    {
        pepper_event_listener_remove(surface->buffer.destroy_listener);
        pepper_event_listener_remove(surface->buffer.release_listener);
        pepper_buffer_unreference(surface->buffer.buffer);
    }

    pixman_region32_fini(&surface->damage_region);
    pixman_region32_fini(&surface->opaque_region);
    pixman_region32_fini(&surface->input_region);

    pepper_list_remove(&surface->link);

    wl_resource_for_each_safe(callback, nc, &surface->frame_callback_list)
        wl_resource_destroy(callback);

    if (surface->role)
        free(surface->role);

    free(surface);
}

static void
attach_surface_to_outputs(pepper_surface_t *surface)
{
    pepper_output_t *output;

    pepper_list_for_each(output, &surface->compositor->output_list, link)
    {
        int              w, h;

        output->backend->attach_surface(output->data, surface, &w, &h);

        if (surface->buffer.buffer)
        {
            surface->buffer.buffer->w = w;
            surface->buffer.buffer->h = h;
        }
    }
}

void
pepper_surface_commit(pepper_surface_t *surface)
{
    pepper_view_t *view;

    /* surface.attach(). */
    if (surface->pending.newly_attached)
    {
        if (surface->buffer.buffer)
        {
            pepper_event_listener_remove(surface->buffer.destroy_listener);
            pepper_event_listener_remove(surface->buffer.release_listener);

            if (!surface->buffer.flushed)
                pepper_buffer_unreference(surface->buffer.buffer);
        }

        if (surface->pending.buffer)
        {
            pepper_event_listener_remove(surface->pending.buffer_destroy_listener);
            pepper_buffer_reference(surface->pending.buffer);

            surface->buffer.release_listener =
                pepper_object_add_event_listener(&surface->pending.buffer->base,
                                                 PEPPER_EVENT_BUFFER_RELEASE, 0,
                                                 surface_handle_buffer_release, surface);

            surface->buffer.destroy_listener =
                pepper_object_add_event_listener(&surface->pending.buffer->base,
                                                 PEPPER_EVENT_OBJECT_DESTROY, 0,
                                                 surface_handle_buffer_destroy, surface);
        }

        surface->buffer.buffer   = surface->pending.buffer;
        surface->buffer.x       += surface->pending.x;
        surface->buffer.y       += surface->pending.y;
        surface->buffer.flushed  = PEPPER_FALSE;

        surface->pending.newly_attached = PEPPER_FALSE;
        surface->pending.buffer = NULL;

        /* Attach to all outputs. */
        attach_surface_to_outputs(surface);
    }

    /* surface.set_buffer_transform(), surface.set_buffer_scale(). */
    surface->buffer.transform  = surface->pending.transform;
    surface->buffer.scale      = surface->pending.scale;

    surface_update_size(surface);

    /* surface.frame(). */
    wl_list_insert_list(&surface->frame_callback_list, &surface->pending.frame_callback_list);
    wl_list_init(&surface->pending.frame_callback_list);

    /* surface.damage(). */
    pixman_region32_copy(&surface->damage_region, &surface->pending.damage_region);
    pixman_region32_clear(&surface->pending.damage_region);

    /* surface.set_opaque_region(), surface.set_input_region(). */
    pixman_region32_copy(&surface->opaque_region, &surface->pending.opaque_region);
    pixman_region32_copy(&surface->input_region, &surface->pending.input_region);

    pepper_list_for_each(view, &surface->view_list, surface_link)
    {
        /* TODO: Option for enabling/disabling auto resize */
        pepper_view_resize(view, surface->w, surface->h);
        pepper_view_mark_dirty(view, PEPPER_VIEW_CONTENT_DIRTY);
    }

    pepper_object_emit_event(&surface->base, PEPPER_EVENT_SURFACE_COMMIT, NULL);
}

void
pepper_surface_send_frame_callback_done(pepper_surface_t *surface, uint32_t time)
{
    struct wl_resource *callback, *next;

    wl_resource_for_each_safe(callback, next, &surface->frame_callback_list)
    {
        wl_callback_send_done(callback, time);
        wl_resource_destroy(callback);
    }
}

PEPPER_API struct wl_resource *
pepper_surface_get_resource(pepper_surface_t *surface)
{
    return surface->resource;
}

PEPPER_API pepper_compositor_t *
pepper_surface_get_compositor(pepper_surface_t *surface)
{
    return surface->compositor;
}

PEPPER_API const char *
pepper_surface_get_role(pepper_surface_t *surface)
{
    return surface->role;
}

PEPPER_API pepper_bool_t
pepper_surface_set_role(pepper_surface_t *surface, const char *role)
{
    if (surface->role)
        return PEPPER_FALSE;

    surface->role = strdup(role);
    if (!surface->role)
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

PEPPER_API pepper_buffer_t *
pepper_surface_get_buffer(pepper_surface_t *surface)
{
    return surface->buffer.buffer;
}

PEPPER_API void
pepper_surface_get_buffer_offset(pepper_surface_t *surface, int32_t *x, int32_t *y)
{
    if (x)
        *x = surface->buffer.x;

    if (y)
        *y = surface->buffer.y;
}

PEPPER_API int32_t
pepper_surface_get_buffer_scale(pepper_surface_t *surface)
{
    return surface->buffer.scale;
}

PEPPER_API int32_t
pepper_surface_get_buffer_transform(pepper_surface_t *surface)
{
    return surface->buffer.transform;
}

PEPPER_API pixman_region32_t *
pepper_surface_get_damage_region(pepper_surface_t *surface)
{
    return &surface->damage_region;
}

PEPPER_API pixman_region32_t *
pepper_surface_get_opaque_region(pepper_surface_t *surface)
{
    return &surface->opaque_region;
}

PEPPER_API pixman_region32_t *
pepper_surface_get_input_region(pepper_surface_t *surface)
{
    return &surface->input_region;
}

PEPPER_API void
pepper_surface_send_enter(pepper_surface_t *surface, pepper_output_t *output)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(surface->resource);

    wl_resource_for_each(resource, &output->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_surface_send_enter(surface->resource, resource);
    }
}

PEPPER_API void
pepper_surface_send_leave(pepper_surface_t *surface, pepper_output_t *output)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(surface->resource);

    wl_resource_for_each(resource, &output->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_surface_send_leave(surface->resource, resource);
    }
}

void
pepper_surface_flush_damage(pepper_surface_t *surface)
{
    pepper_view_t      *view;
    pepper_output_t    *output;

    if (!pixman_region32_not_empty(&surface->damage_region))
        return;

    pepper_list_for_each(view, &surface->view_list, surface_link)
        pepper_view_surface_damage(view);

    pepper_list_for_each(output, &surface->compositor->output_list, link)
        output->backend->flush_surface_damage(output->data, surface);

    pixman_region32_clear(&surface->damage_region);

    if (surface->buffer.buffer)
    {
        pepper_buffer_unreference(surface->buffer.buffer);
        surface->buffer.flushed = PEPPER_TRUE;
    }
}
