#include "pepper-internal.h"
#include <float.h>

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
    pepper_view_t *view = pepper_compositor_pick_view(pointer->seat->compositor,
                                                      pointer->x, pointer->y,
                                                      &pointer->vx, &pointer->vy);

    pepper_pointer_set_focus(pointer, view);
    pepper_pointer_send_motion(pointer, time, pointer->vx, pointer->vy);
}

static void
default_pointer_grab_button(pepper_pointer_t *pointer, void *data,
                            uint32_t time, uint32_t button, uint32_t state)
{
    if (pointer->seat->keyboard && button == BTN_LEFT && state == PEPPER_BUTTON_STATE_PRESSED)
    {
        pepper_view_t *focus = pointer->focus;

        pepper_keyboard_set_focus(pointer->seat->keyboard, focus);

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

static pepper_bool_t
pointer_clamp(pepper_pointer_t *pointer)
{
    pepper_bool_t clamped = PEPPER_FALSE;

    if (pointer->x < pointer->clamp.x0)
    {
        pointer->x = pointer->clamp.x0;
        clamped = PEPPER_TRUE;
    }

    if (pointer->x > pointer->clamp.x1)
    {
        pointer->x = pointer->clamp.x1;
        clamped = PEPPER_TRUE;
    }

    if (pointer->y < pointer->clamp.y0)
    {
        pointer->y = pointer->clamp.y0;
        clamped = PEPPER_TRUE;
    }

    if (pointer->y > pointer->clamp.y1)
    {
        pointer->y = pointer->clamp.y1;
        clamped = PEPPER_TRUE;
    }

    return clamped;
}

static void
pointer_set_position(pepper_pointer_t *pointer, uint32_t time, double x, double y)
{
    pepper_input_event_t event;

    if (x == pointer->x && y == pointer->y)
        return;

    pointer->x = x;
    pointer->y = y;

    pointer_clamp(pointer);

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
        pointer_set_position(pointer, event->time,
                             pointer->x + event->x * pointer->x_velocity,
                             pointer->y + event->y * pointer->y_velocity);
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
pointer_handle_focus_destroy(struct wl_listener *listener, void *data)
{
    pepper_pointer_t *pointer = pepper_container_of(listener, pointer, focus_destroy_listener);
    pepper_pointer_set_focus(pointer, NULL);
    pointer->grab->cancel(pointer, pointer->data);
}

pepper_pointer_t *
pepper_pointer_create(pepper_seat_t *seat)
{
    pepper_pointer_t *pointer =
        (pepper_pointer_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_pointer_t));

    PEPPER_CHECK(pointer, return NULL, "pepper_object_alloc() failed.\n");

    pointer->seat = seat;
    wl_list_init(&pointer->resource_list);
    pointer->focus_destroy_listener.notify = pointer_handle_focus_destroy;

    pointer->grab = &default_pointer_grab;

    pointer->clamp.x0 = DBL_MIN;
    pointer->clamp.y0 = DBL_MIN;
    pointer->clamp.x1 = DBL_MAX;
    pointer->clamp.y1 = DBL_MAX;

    pointer->x_velocity = 1.0;
    pointer->y_velocity = 1.0;

    return pointer;
}

void
pepper_pointer_destroy(pepper_pointer_t *pointer)
{
    pointer->grab->cancel(pointer, pointer->data);

    if (pointer->focus)
        wl_list_remove(&pointer->focus_destroy_listener.link);

    free(pointer);
}

static void
unbind_resource(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

void
pepper_pointer_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    pepper_pointer_t   *pointer = seat->pointer;
    struct wl_resource *res;

    if (!pointer)
        return;

    res = wl_resource_create(client, &wl_pointer_interface, wl_resource_get_version(resource), id);
    if (!res)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&pointer->resource_list, wl_resource_get_link(res));
    wl_resource_set_implementation(res, &pointer_impl, pointer, unbind_resource);

    if (!pointer->focus)
        return;

    if (wl_resource_get_client(pointer->focus->surface->resource) == client)
    {
        wl_pointer_send_enter(res, pointer->focus_serial,
                              pointer->focus->surface->resource,
                              wl_fixed_from_double(pointer->vx), wl_fixed_from_double(pointer->vy));
    }
}

PEPPER_API pepper_bool_t
pepper_pointer_set_clamp(pepper_pointer_t *pointer, double x0, double y0, double x1, double y1)
{
    if (x1 < x0 || y1 < y0)
        return PEPPER_FALSE;

    pointer->clamp.x0 = x0;
    pointer->clamp.y0 = y0;
    pointer->clamp.x1 = x1;
    pointer->clamp.y1 = y1;

    if (pointer_clamp(pointer))
    {
        pepper_input_event_t event;

        pointer->grab->motion(pointer, pointer->data, pointer->time, pointer->x, pointer->y);

        memset(&event, 0x00, sizeof(pepper_input_event_t));
        event.id = PEPPER_EVENT_POINTER_MOTION;
        event.time = pointer->time;
        event.x = pointer->x;
        event.y = pointer->y;
        pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_MOTION, &event);
    }

    return PEPPER_TRUE;
}

PEPPER_API void
pepper_pointer_get_clamp(pepper_pointer_t *pointer, double *x0, double *y0, double *x1, double *y1)
{
    if (x0)
        *x0 = pointer->clamp.x0;

    if (y0)
        *y0 = pointer->clamp.y0;

    if (x1)
        *x1 = pointer->clamp.x1;

    if (y1)
        *y1 = pointer->clamp.y1;
}

PEPPER_API void
pepper_pointer_set_velocity(pepper_pointer_t *pointer, double vx, double vy)
{
    pointer->x_velocity = vx;
    pointer->y_velocity = vy;
}

PEPPER_API void
pepper_pointer_get_velocity(pepper_pointer_t *pointer, double *vx, double *vy)
{
    if (vx)
        *vx = pointer->x_velocity;

    if (vy)
        *vy = pointer->y_velocity;
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
    if (pointer->focus == focus)
        return;

    if (pointer->focus)
    {
        pepper_pointer_send_leave(pointer);
        wl_list_remove(&pointer->focus_destroy_listener.link);
        pepper_object_emit_event(&pointer->base, PEPPER_EVENT_FOCUS_LEAVE, pointer->focus);
        pepper_object_emit_event(&pointer->focus->base, PEPPER_EVENT_FOCUS_LEAVE, pointer);
    }

    pointer->focus = focus;

    if (focus)
    {
        pepper_pointer_send_enter(pointer, pointer->vx, pointer->vy);
        wl_resource_add_destroy_listener(focus->surface->resource, &pointer->focus_destroy_listener);
        pointer->focus_serial = wl_display_next_serial(pointer->seat->compositor->display);
        pepper_object_emit_event(&pointer->base, PEPPER_EVENT_FOCUS_ENTER, focus);
        pepper_object_emit_event(&focus->base, PEPPER_EVENT_FOCUS_ENTER, pointer);
    }
}

PEPPER_API pepper_view_t *
pepper_pointer_get_focus(pepper_pointer_t *pointer)
{
    return pointer->focus;
}

PEPPER_API void
pepper_pointer_send_leave(pepper_pointer_t *pointer)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(pointer->focus->surface->resource);
    uint32_t            serial = wl_display_next_serial(pointer->seat->compositor->display);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_leave(resource, serial, pointer->focus->surface->resource);
    }
}

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, double x, double y)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(pointer->focus->surface->resource);
    uint32_t            serial = wl_display_next_serial(pointer->seat->compositor->display);
    wl_fixed_t          fx = wl_fixed_from_double(x);
    wl_fixed_t          fy = wl_fixed_from_double(y);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_enter(resource, serial, pointer->focus->surface->resource, fx, fy);
    }
}

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, uint32_t time, double x, double y)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(pointer->focus->surface->resource);
    wl_fixed_t          fx = wl_fixed_from_double(x);
    wl_fixed_t          fy = wl_fixed_from_double(y);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_motion(resource, time, fx, fy);
    }
}

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, uint32_t time, uint32_t button, uint32_t state)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(pointer->focus->surface->resource);
    uint32_t            serial = wl_display_next_serial(pointer->seat->compositor->display);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_button(resource, serial, time, button, state);
    }
}

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, uint32_t time, uint32_t axis, double value)
{
    struct wl_resource *resource;
    struct wl_client   *client = wl_resource_get_client(pointer->focus->surface->resource);
    wl_fixed_t          v = wl_fixed_from_double(value);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_axis(resource, time, axis, v);
    }
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
