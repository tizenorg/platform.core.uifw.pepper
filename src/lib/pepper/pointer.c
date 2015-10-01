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

static void
default_pointer_grab_motion(pepper_pointer_t *pointer, void *data, uint32_t time, double x, double y)
{
    pepper_view_t *view = pepper_compositor_pick_view(pointer->input.seat->compositor,
                                                      pointer->x, pointer->y,
                                                      &pointer->vx, &pointer->vy);

    pepper_pointer_set_focus(pointer, view);
    pepper_pointer_send_motion(pointer, time, pointer->vx, pointer->vy);
}

static void
default_pointer_grab_button(pepper_pointer_t *pointer, void *data,
                            uint32_t time, uint32_t button, uint32_t state)
{
    if (pointer->input.seat->keyboard && button == BTN_LEFT && state == PEPPER_BUTTON_STATE_PRESSED)
    {
        pepper_view_t *focus = pointer->input.focus;

        pepper_keyboard_set_focus(pointer->input.seat->keyboard, focus);

        if (focus)
            pepper_view_stack_top(focus, PEPPER_FALSE);
    }

    pepper_pointer_send_button(pointer, time, button, state);
}

static void
default_pointer_grab_axis(pepper_pointer_t *pointer, void *data,
                          uint32_t time, uint32_t axis, double value)
{
    pepper_pointer_send_axis(pointer, time, axis, value);
}

static void
default_pointer_grab_cancel(pepper_pointer_t *pointer, void *data)
{
    /* Nothing to do.*/
}

static const pepper_pointer_grab_t default_pointer_grab =
{
    default_pointer_grab_motion,
    default_pointer_grab_button,
    default_pointer_grab_axis,
    default_pointer_grab_cancel,
};

static void
pointer_set_position(pepper_pointer_t *pointer, uint32_t time, double x, double y)
{
    pepper_input_event_t event;

    pointer->x = x;
    pointer->y = y;

    pointer->grab->motion(pointer, pointer->data, time, x, y);

    /* Emit motion event. */
    memset(&event, 0x00, sizeof(pepper_input_event_t));
    event.id = PEPPER_EVENT_POINTER_MOTION;
    event.time = time;
    event.x = x;
    event.y = y;
    pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_MOTION, &event);
}

void
pepper_pointer_handle_event(pepper_pointer_t *pointer, uint32_t id, pepper_input_event_t *event)
{
    switch (id)
    {
    case PEPPER_EVENT_POINTER_MOTION_ABSOLUTE:
        pointer_set_position(pointer, event->time, event->x, event->y);
        break;
    case PEPPER_EVENT_POINTER_MOTION:
        pointer_set_position(pointer, event->time, pointer->x + event->x, pointer->y + event->y);
        break;
    case PEPPER_EVENT_POINTER_BUTTON:
        pointer->grab->button(pointer, pointer->data, event->time, event->button, event->state);
        pepper_object_emit_event(&pointer->base, id, event);
        break;
    case PEPPER_EVENT_POINTER_AXIS:
        pointer->grab->axis(pointer, pointer->data, event->time, event->axis, event->value);
        pepper_object_emit_event(&pointer->base, id, event);
        break;
    }
}

static void
pointer_handle_focus_destroy(pepper_object_t *object, void *data)
{
    pepper_pointer_t *pointer = (pepper_pointer_t *)object;
    pointer->grab->cancel(pointer, pointer->data);
}

pepper_pointer_t *
pepper_pointer_create(pepper_seat_t *seat)
{
    pepper_pointer_t *pointer =
        (pepper_pointer_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_pointer_t));

    PEPPER_CHECK(pointer, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&pointer->input, seat, &pointer->base, pointer_handle_focus_destroy);
    pointer->grab = &default_pointer_grab;

    return pointer;
}

void
pepper_pointer_destroy(pepper_pointer_t *pointer)
{
    pointer->grab->cancel(pointer, pointer->data);
    pepper_input_fini(&pointer->input);
    free(pointer);
}

void
pepper_pointer_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    pepper_pointer_t   *pointer = seat->pointer;
    struct wl_resource *res;

    if (!pointer)
        return;

    res = pepper_input_bind_resource(&pointer->input, client, wl_resource_get_version(resource),
                                     id, &wl_pointer_interface, &pointer_impl, pointer);
    PEPPER_CHECK(res, return, "pepper_input_bind_resource() failed.\n");

    if (!pointer->input.focus)
        return;

    if (wl_resource_get_client(pointer->input.focus->surface->resource) == client)
    {
        wl_pointer_send_enter(res, pointer->input.focus_serial,
                              pointer->input.focus->surface->resource,
                              wl_fixed_from_double(pointer->vx), wl_fixed_from_double(pointer->vy));
    }
}

PEPPER_API void
pepper_pointer_get_position(pepper_pointer_t *pointer, double *x, double *y)
{
    if (x)
        *x = pointer->x;

    if (y)
        *y = pointer->y;
}

PEPPER_API void
pepper_pointer_set_focus(pepper_pointer_t *pointer, pepper_view_t *focus)
{
    if (pointer->input.focus == focus)
        return;

    if (pointer->input.focus)
        pepper_pointer_send_leave(pointer);

    pepper_input_set_focus(&pointer->input, focus);

    if (pointer->input.focus)
        pepper_pointer_send_enter(pointer, pointer->vx, pointer->vy);
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
}

PEPPER_API void
pepper_pointer_end_grab(pepper_pointer_t *pointer)
{
    pointer->grab = &default_pointer_grab;
}
