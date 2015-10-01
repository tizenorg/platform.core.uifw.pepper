#include "pepper-internal.h"

static void
touch_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_touch_interface touch_impl =
{
    touch_release,
};

static void
touch_handle_focus_destroy(pepper_object_t *object, void *data)
{
    pepper_touch_t *touch = (pepper_touch_t *)object;

    if (touch->grab)
        touch->grab->cancel(touch, touch->data);
}

void
pepper_touch_handle_event(pepper_touch_t *touch, uint32_t id, pepper_input_event_t *event)
{
    switch (id)
    {
    case PEPPER_EVENT_TOUCH_DOWN:
        touch->grab->down(touch, touch->data, event->time, event->id, event->x, event->y);
        break;
    case PEPPER_EVENT_TOUCH_UP:
        touch->grab->up(touch, touch->data, event->time, event->id);
        break;
    case PEPPER_EVENT_TOUCH_MOTION:
        touch->grab->motion(touch, touch->data, event->time, event->id, event->x, event->y);
        break;
    case PEPPER_EVENT_TOUCH_FRAME:
        touch->grab->frame(touch, touch->data);
        break;
    case PEPPER_EVENT_TOUCH_CANCEL:
        touch->grab->cancel(touch, touch->data);
        break;
    }

    pepper_object_emit_event(&touch->base, id, event);
}

pepper_touch_t *
pepper_touch_create(pepper_seat_t *seat)
{
    pepper_touch_t *touch =
        (pepper_touch_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_touch_t));

    PEPPER_CHECK(touch, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&touch->input, seat, &touch->base, touch_handle_focus_destroy);
    return touch;
}

void
pepper_touch_destroy(pepper_touch_t *touch)
{
    if (touch->grab)
        touch->grab->cancel(touch, touch->data);

    pepper_input_fini(&touch->input);
    free(touch);
}

void
pepper_touch_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    pepper_touch_t     *touch = seat->touch;
    struct wl_resource *res;

    if (!touch)
        return;

    res = pepper_input_bind_resource(&touch->input, client, wl_resource_get_version(resource),
                                     id, &wl_touch_interface, &touch_impl, touch);
    PEPPER_CHECK(res, return, "pepper_input_bind_resource() failed.\n");
}

PEPPER_API void
pepper_touch_set_focus(pepper_touch_t *touch, pepper_view_t *focus)
{
    pepper_input_set_focus(&touch->input, focus);
}

PEPPER_API pepper_view_t *
pepper_touch_get_focus(pepper_touch_t *touch)
{
    return touch->input.focus;
}

PEPPER_API void
pepper_touch_send_down(pepper_touch_t *touch, uint32_t time, uint32_t id, double x, double y)
{
    if (!wl_list_empty(&touch->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(touch->input.seat->compositor->display);

        wl_resource_for_each(resource, &touch->input.focus_resource_list)
        {
            wl_touch_send_down(resource, serial, time, touch->input.focus->surface->resource,
                               id, wl_fixed_from_double(x), wl_fixed_from_double(y));
        }
    }
}

PEPPER_API void
pepper_touch_send_up(pepper_touch_t *touch, uint32_t time, uint32_t id)
{
    if (!wl_list_empty(&touch->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(touch->input.seat->compositor->display);

        wl_resource_for_each(resource, &touch->input.focus_resource_list)
            wl_touch_send_up(resource, serial, time, id);
    }
}

PEPPER_API void
pepper_touch_send_motion(pepper_touch_t *touch, uint32_t time, uint32_t id, double x, double y)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &touch->input.focus_resource_list)
        wl_touch_send_motion(resource, time, id, wl_fixed_from_double(x), wl_fixed_from_double(y));
}

PEPPER_API void
pepper_touch_send_frame(pepper_touch_t *touch)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &touch->input.focus_resource_list)
        wl_touch_send_frame(resource);
}

PEPPER_API void
pepper_touch_send_cancel(pepper_touch_t *touch)
{
    struct wl_resource *resource;

    wl_resource_for_each(resource, &touch->input.focus_resource_list)
        wl_touch_send_cancel(resource);
}

PEPPER_API void
pepper_touch_start_grab(pepper_touch_t *touch, const pepper_touch_grab_t *grab, void *data)
{
    touch->grab = grab;
    touch->data = data;
}

PEPPER_API void
pepper_touch_end_grab(pepper_touch_t *touch)
{
    /* TODO: switch back to default grab. */
    touch->grab = NULL;
}
