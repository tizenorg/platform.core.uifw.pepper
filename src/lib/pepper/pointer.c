#include "pepper-internal.h"

static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial,
                   struct wl_resource *surface_resource, int32_t x, int32_t y)
{
    /* TODO: */
}

static void
pointer_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_pointer_interface pointer_impl =
{
    pointer_set_cursor,
    pointer_release,
};

pepper_pointer_t *
pepper_pointer_create(pepper_seat_t *seat)
{
    pepper_pointer_t *pointer =
        (pepper_pointer_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_pointer_t));

    PEPPER_CHECK(pointer, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&pointer->input, seat);
    return pointer;
}

void
pepper_pointer_destroy(pepper_pointer_t *pointer)
{
    pepper_input_fini(&pointer->input);
    free(pointer);
}

void
pepper_pointer_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);

    if (!seat->pointer)
        return;

    pepper_input_bind_resource(&seat->pointer->input, client, wl_resource_get_version(resource),
                               id, &wl_pointer_interface, &pointer_impl, seat->pointer);
}

PEPPER_API void
pepper_pointer_set_position(pepper_pointer_t *pointer, int32_t x, int32_t y)
{
    /* TODO: */
}

PEPPER_API void
pepper_pointer_get_position(pepper_pointer_t *pointer, int32_t *x, int32_t *y)
{
    /* TODO: */
}

PEPPER_API void
pepper_pointer_set_focus(pepper_pointer_t *pointer, pepper_view_t *focus)
{
    /* TODO: */
}

PEPPER_API pepper_view_t *
pepper_pointer_get_focus(pepper_pointer_t *pointer)
{
    /* TODO: */
    return NULL;
}

PEPPER_API void
pepper_pointer_send_leave(pepper_pointer_t *pointer, pepper_view_t *target_view)
{
    /* TODO: */
}

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, pepper_view_t *target_view)
{
    /* TODO: */
}

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, pepper_view_t *target_view,
                           uint32_t time, int32_t x, int32_t y)
{
    /* TODO: */
}

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, pepper_view_t *target_view,
                           uint32_t time, uint32_t button, uint32_t state)
{
    /* TODO: */
}

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, pepper_view_t *target_view,
                         uint32_t time, uint32_t axis, uint32_t amount)
{
    /* TODO: */
}
