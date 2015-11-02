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

    if (pointer->grab)
        pointer->grab->motion(pointer, pointer->data, time, pointer->x, pointer->y);

    /* Emit motion event. */
    memset(&event, 0x00, sizeof(pepper_input_event_t));
    event.id = PEPPER_EVENT_POINTER_MOTION;
    event.time = time;
    event.x = pointer->x;
    event.y = pointer->y;
    pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_MOTION, &event);
}

void
pepper_pointer_handle_event(pepper_pointer_t *pointer, uint32_t id, pepper_input_event_t *event)
{
    switch (id)
    {
    case PEPPER_EVENT_POINTER_MOTION_ABSOLUTE:
        {
            pointer_set_position(pointer, event->time, event->x, event->y);
        }
        break;
    case PEPPER_EVENT_POINTER_MOTION:
        {
            pointer_set_position(pointer, event->time,
                                 pointer->x + event->x * pointer->x_velocity,
                                 pointer->y + event->y * pointer->y_velocity);
        }
        break;
    case PEPPER_EVENT_POINTER_BUTTON:
        {
            if (pointer->grab)
            {
                pointer->grab->button(pointer, pointer->data,
                                      event->time, event->button, event->state);
            }

            pepper_object_emit_event(&pointer->base, id, event);
        }
        break;
    case PEPPER_EVENT_POINTER_AXIS:
        {
            if (pointer->grab)
                pointer->grab->axis(pointer, pointer->data, event->time, event->axis, event->value);

            pepper_object_emit_event(&pointer->base, id, event);
        }
        break;
    }
}

static void
pointer_handle_focus_destroy(pepper_event_listener_t *listener, pepper_object_t *surface,
                             uint32_t id, void *info, void *data)
{
    pepper_pointer_t *pointer = data;
    pepper_pointer_set_focus(pointer, NULL);

    if (pointer->grab)
        pointer->grab->cancel(pointer, pointer->data);
}

pepper_pointer_t *
pepper_pointer_create(pepper_seat_t *seat)
{
    pepper_pointer_t *pointer =
        (pepper_pointer_t *)pepper_object_alloc(PEPPER_OBJECT_POINTER, sizeof(pepper_pointer_t));

    PEPPER_CHECK(pointer, return NULL, "pepper_object_alloc() failed.\n");

    pointer->seat = seat;
    wl_list_init(&pointer->resource_list);

    pointer->clamp.x0 = -DBL_MAX;
    pointer->clamp.y0 = -DBL_MAX;
    pointer->clamp.x1 = DBL_MAX;
    pointer->clamp.y1 = DBL_MAX;

    pointer->x_velocity = 1.0;
    pointer->y_velocity = 1.0;

    return pointer;
}

void
pepper_pointer_destroy(pepper_pointer_t *pointer)
{
    if (pointer->grab)
        pointer->grab->cancel(pointer, pointer->data);

    if (pointer->focus)
        pepper_event_listener_remove(pointer->focus_destroy_listener);

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

    if (!pointer->focus || !pointer->focus->surface || !pointer->focus->surface->resource)
        return;

    if (wl_resource_get_client(pointer->focus->surface->resource) == client)
    {
        wl_pointer_send_enter(res, pointer->focus_serial,
                              pointer->focus->surface->resource,
                              wl_fixed_from_double(pointer->vx), wl_fixed_from_double(pointer->vy));
    }
}

PEPPER_API struct wl_list *
pepper_pointer_get_resource_list(pepper_pointer_t *pointer)
{
    return &pointer->resource_list;
}

PEPPER_API pepper_compositor_t *
pepper_pointer_get_compositor(pepper_pointer_t *pointer)
{
    return pointer->seat->compositor;
}

PEPPER_API pepper_seat_t *
pepper_pointer_get_seat(pepper_pointer_t *pointer)
{
    return pointer->seat;
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

        if (pointer->grab)
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
        pepper_event_listener_remove(pointer->focus_destroy_listener);
        pepper_object_emit_event(&pointer->base, PEPPER_EVENT_FOCUS_LEAVE, pointer->focus);
        pepper_object_emit_event(&pointer->focus->base, PEPPER_EVENT_FOCUS_LEAVE, pointer);
    }

    pointer->focus = focus;

    if (focus)
    {
        pointer->focus_serial = wl_display_next_serial(pointer->seat->compositor->display);

        pointer->focus_destroy_listener =
            pepper_object_add_event_listener(&focus->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                             pointer_handle_focus_destroy, pointer);

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
pepper_pointer_send_leave(pepper_pointer_t *pointer, pepper_view_t *view)
{
    struct wl_resource *resource;
    struct wl_client   *client;
    uint32_t            serial;

    if (!view || !view->surface || !view->surface->resource)
        return;

    client = wl_resource_get_client(view->surface->resource);
    serial = wl_display_next_serial(pointer->seat->compositor->display);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_leave(resource, serial, view->surface->resource);
    }
}

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, pepper_view_t *view, double x, double y)
{
    struct wl_resource *resource;
    wl_fixed_t          fx = wl_fixed_from_double(x);
    wl_fixed_t          fy = wl_fixed_from_double(y);
    struct wl_client   *client;
    uint32_t            serial;

    if (!view || !view->surface || !view->surface->resource)
        return;

    client = wl_resource_get_client(view->surface->resource);
    serial = wl_display_next_serial(pointer->seat->compositor->display);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_enter(resource, serial, view->surface->resource, fx, fy);
    }
}

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, pepper_view_t *view,
                           uint32_t time, double x, double y)
{
    struct wl_resource *resource;
    wl_fixed_t          fx = wl_fixed_from_double(x);
    wl_fixed_t          fy = wl_fixed_from_double(y);
    struct wl_client   *client;

    if (!view || !view->surface || !view->surface->resource)
        return;

    client = wl_resource_get_client(view->surface->resource);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_motion(resource, time, fx, fy);
    }
}

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, pepper_view_t *view,
                           uint32_t time, uint32_t button, uint32_t state)
{
    struct wl_resource *resource;
    struct wl_client   *client;
    uint32_t            serial;

    if (!view || !view->surface || !view->surface->resource)
        return;

    client = wl_resource_get_client(view->surface->resource);
    serial = wl_display_next_serial(pointer->seat->compositor->display);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_button(resource, serial, time, button, state);
    }
}

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, pepper_view_t *view,
                         uint32_t time, uint32_t axis, double value)
{
    struct wl_resource *resource;
    wl_fixed_t          v = wl_fixed_from_double(value);
    struct wl_client   *client;

    if (!view || !view->surface || !view->surface->resource)
        return;

    client = wl_resource_get_client(view->surface->resource);

    wl_resource_for_each(resource, &pointer->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_pointer_send_axis(resource, time, axis, v);
    }
}

PEPPER_API void
pepper_pointer_set_grab(pepper_pointer_t *pointer, const pepper_pointer_grab_t *grab, void *data)
{
    pointer->grab = grab;
    pointer->data = data;
}

PEPPER_API const pepper_pointer_grab_t *
pepper_pointer_get_grab(pepper_pointer_t *pointer)
{
    return pointer->grab;
}

PEPPER_API void *
pepper_pointer_get_grab_data(pepper_pointer_t *pointer)
{
    return pointer->data;
}
