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
    if (pointer->grab)
        pointer->grab->cancel(pointer, pointer->data);

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
    pepper_pointer_send_leave(pointer);
    pepper_input_set_focus(&pointer->input, focus);

    /* TODO: Calculate surface local coordinate. */
    pepper_pointer_send_enter(pointer, 0.0, 0.0);
}

PEPPER_API pepper_view_t *
pepper_pointer_get_focus(pepper_pointer_t *pointer)
{
    return pointer->input.focus;
}

PEPPER_API void
pepper_pointer_send_leave(pepper_pointer_t *pointer)
{
    if (!wl_list_empty(&pointer->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(pointer->input.seat->compositor->display);

        wl_resource_for_each(resource, &pointer->input.focus_resource_list)
            wl_pointer_send_leave(resource, serial, pointer->input.focus->surface->resource);
    }
}

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, double x, double y)
{
    if (!wl_list_empty(&pointer->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(pointer->input.seat->compositor->display);

        wl_resource_for_each(resource, &pointer->input.focus_resource_list)
        {
            wl_pointer_send_enter(resource, serial, pointer->input.focus->surface->resource,
                                  wl_fixed_from_double(x), wl_fixed_from_double(y));
        }
    }
}

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, uint32_t time, double x, double y)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &pointer->input.focus_resource_list)
        wl_pointer_send_motion(resource, time, wl_fixed_from_double(x), wl_fixed_from_double(y));
}

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, uint32_t time, uint32_t button, uint32_t state)
{
    if (!wl_list_empty(&pointer->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(pointer->input.seat->compositor->display);

        wl_resource_for_each(resource, &pointer->input.focus_resource_list)
            wl_pointer_send_button(resource, serial, time, button, state);
    }
}

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, uint32_t time, uint32_t axis, double value)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &pointer->input.focus_resource_list)
        wl_pointer_send_axis(resource, time, axis, wl_fixed_from_double(value));
}

PEPPER_API void
pepper_pointer_start_grab(pepper_pointer_t *pointer, const pepper_pointer_grab_t *grab, void *data)
{
    pointer->grab = grab;
    pointer->data = data;

    if (grab)
        grab->focus(pointer, pointer->data);
}

PEPPER_API void
pepper_pointer_end_grab(pepper_pointer_t *pointer)
{
    /* TODO: switch back to default grab. */
    pointer->grab = NULL;
    pointer->grab->focus(pointer, NULL);
}
