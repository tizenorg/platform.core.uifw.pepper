#include "pepper-internal.h"

static void
buffer_destroy_handler(struct wl_listener *listener, void *data)
{
    pepper_buffer_t        *buffer = data;
    pepper_surface_state_t *state = wl_container_of(listener, state, buffer_destroy_listener);

    state->buffer = NULL;
}

static void
pepper_surface_state_init(pepper_surface_state_t *state)
{
    state->buffer = NULL;
    state->x = 0;
    state->y = 0;
    state->transform = WL_OUTPUT_TRANSFORM_NORMAL;
    state->scale = 1;

    pixman_region32_init(&state->damage_region);
    pixman_region32_init(&state->opaque_region);
    pixman_region32_init(&state->input_region);

    wl_list_init(&state->frame_callbacks);
    state->buffer_destroy_listener.notify = buffer_destroy_handler;
}

static void
pepper_surface_state_fini(pepper_surface_state_t *state)
{
    struct wl_resource *callback, *next;

    pixman_region32_fini(&state->damage_region);
    pixman_region32_fini(&state->opaque_region);
    pixman_region32_fini(&state->input_region);

    wl_resource_for_each_safe(callback, next, &state->frame_callbacks)
        wl_resource_destroy(callback);
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
        wl_list_remove(&surface->pending.buffer_destroy_listener.link);

    surface->pending.buffer = buffer;
    surface->pending.x = x;
    surface->pending.y = y;

    if (buffer)
        wl_signal_add(&buffer->destroy_signal, &surface->pending.buffer_destroy_listener);
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
    wl_list_insert(surface->pending.frame_callbacks.prev, wl_resource_get_link(callback));
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
        pixman_region32_clear(&surface->pending.input_region);
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
    pepper_surface_t *surface;

    surface = (pepper_surface_t *)pepper_calloc(1, sizeof(pepper_surface_t));

    if (!surface)
    {
        PEPPER_ERROR("Surface memory allocation failed\n");
        wl_resource_post_no_memory(resource);
        return NULL;
    }

    surface->compositor = compositor;
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

    wl_resource_set_implementation(surface->resource, &surface_implementation, surface,
                                   surface_resource_destroy_handler);
    wl_list_insert(&compositor->surfaces, wl_resource_get_link(surface->resource));

    pepper_surface_state_init(&surface->pending);

    surface->transform = WL_OUTPUT_TRANSFORM_NORMAL;
    surface->scale = 1;

    pixman_region32_init(&surface->damage_region);
    pixman_region32_init(&surface->opaque_region);
    pixman_region32_init(&surface->input_region);

    wl_list_init(&surface->frame_callbacks);
    wl_signal_init(&surface->destroy_signal);

    return surface;
}

void
pepper_surface_destroy(pepper_surface_t *surface)
{
    struct wl_resource *callback, *next;

    wl_signal_emit(&surface->destroy_signal, NULL /* FIXME */);

    pepper_surface_state_fini(&surface->pending);

    if (surface->buffer)
        pepper_buffer_unreference(surface->buffer);

    pixman_region32_fini(&surface->damage_region);
    pixman_region32_fini(&surface->opaque_region);
    pixman_region32_fini(&surface->input_region);

    wl_resource_for_each_safe(callback, next, &surface->frame_callbacks)
        wl_resource_destroy(callback);
}

void
pepper_surface_commit(pepper_surface_t *surface)
{
    pepper_bool_t need_redraw = PEPPER_FALSE;

    pepper_buffer_reference(surface->pending.buffer);

    if (surface->buffer)
        pepper_buffer_unreference(surface->buffer);

    /* Commit buffer attachment states. */
    surface->buffer     = surface->pending.buffer;
    surface->offset_x  += surface->pending.x;
    surface->offset_y  += surface->pending.y;
    surface->transform  = surface->pending.transform;
    surface->scale      = surface->pending.scale;

    /* Stop listening on buffer destruction signal of the pending state. */
    if (surface->pending.buffer)
        wl_list_remove(&surface->pending.buffer_destroy_listener.link);

    /* Migrate frame callbacks into current state. */
    wl_list_insert_list(&surface->frame_callbacks, &surface->pending.frame_callbacks);
    wl_list_init(&surface->pending.frame_callbacks);

    /* Calculate surface size. */
    surface->w = 0;
    surface->h = 0;

    if (surface->buffer)
    {
        switch (surface->transform)
        {
        case WL_OUTPUT_TRANSFORM_NORMAL:
        case WL_OUTPUT_TRANSFORM_180:
        case WL_OUTPUT_TRANSFORM_FLIPPED_180:
            surface->w = surface->buffer->w;
            surface->h = surface->buffer->h;
            break;
        case WL_OUTPUT_TRANSFORM_90:
        case WL_OUTPUT_TRANSFORM_270:
        case WL_OUTPUT_TRANSFORM_FLIPPED_90:
        case WL_OUTPUT_TRANSFORM_FLIPPED_270:
            surface->w = surface->buffer->h;
            surface->h = surface->buffer->w;
            break;
        }
    }

    surface->w /= surface->scale;
    surface->h /= surface->scale;

    /* Commit damage region. Pending state's damage is consumed by commit. */
    pixman_region32_union(&surface->damage_region,
                          &surface->damage_region, &surface->pending.damage_region);
    pixman_region32_clear(&surface->pending.damage_region);

    /* Are we safe here just pointing to the region objects in the pending states?? */
    pixman_region32_copy(&surface->opaque_region, &surface->pending.opaque_region);
    pixman_region32_copy(&surface->input_region, &surface->pending.input_region);

    /* TODO: Now schedule redraw. */
}
