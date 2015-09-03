#include "pepper-internal.h"

static void
buffer_resource_destroy_handler(struct wl_listener *listener, void *data)
{
    pepper_buffer_t *buffer = pepper_container_of(listener, buffer, resource_destroy_listener);
    pepper_object_fini(&buffer->base);
    free(buffer);
}

pepper_buffer_t *
pepper_buffer_from_resource(struct wl_resource *resource)
{
    pepper_buffer_t     *buffer;
    struct wl_listener  *listener;

    listener = wl_resource_get_destroy_listener(resource, buffer_resource_destroy_handler);

    if (listener)
        return pepper_container_of(listener, buffer, resource_destroy_listener);

    buffer = (pepper_buffer_t *)pepper_object_alloc(PEPPER_OBJECT_BUFFER, sizeof(pepper_buffer_t));
    if (!buffer)
        return NULL;

    buffer->resource = resource;
    buffer->resource_destroy_listener.notify = buffer_resource_destroy_handler;
    wl_resource_add_destroy_listener(resource, &buffer->resource_destroy_listener);

    return buffer;
}

PEPPER_API void
pepper_buffer_reference(pepper_buffer_t *buffer)
{
    PEPPER_ASSERT(buffer->ref_count >= 0);

    buffer->ref_count++;
}

PEPPER_API void
pepper_buffer_unreference(pepper_buffer_t *buffer)
{
    PEPPER_ASSERT(buffer->ref_count > 0);

    if (--buffer->ref_count == 0)
    {
        wl_resource_queue_event(buffer->resource, WL_BUFFER_RELEASE);
        pepper_object_emit_event(&buffer->base, PEPPER_EVENT_BUFFER_RELEASE, NULL);
    }
}

PEPPER_API struct wl_resource *
pepper_buffer_get_resource(pepper_buffer_t *buffer)
{
    return buffer->resource;
}
