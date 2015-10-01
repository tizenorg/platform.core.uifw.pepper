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

    touch->seat = seat;
    wl_list_init(&touch->resource_list);

    return touch;
}

void
pepper_touch_destroy(pepper_touch_t *touch)
{
    if (touch->grab)
        touch->grab->cancel(touch, touch->data);

    free(touch);
}

static void
unbind_resource(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

void
pepper_touch_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    pepper_touch_t     *touch = seat->touch;
    struct wl_resource *res;

    if (!touch)
        return;

    res = wl_resource_create(client, &wl_touch_interface, wl_resource_get_version(resource), id);
    if (!res)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&touch->resource_list, wl_resource_get_link(res));
    wl_resource_set_implementation(res, &touch_impl, touch, unbind_resource);

    /* TODO: Send down for newly bound resources. */
}

PEPPER_API struct wl_list *
pepper_touch_get_resource_list(pepper_touch_t *touch)
{
    return &touch->resource_list;
}

PEPPER_API pepper_compositor_t *
pepper_touch_get_compositor(pepper_touch_t *touch)
{
    return touch->seat->compositor;
}

PEPPER_API void
pepper_touch_set_focus(pepper_touch_t *touch, pepper_view_t *focus)
{
    /* TODO: */
}

PEPPER_API pepper_view_t *
pepper_touch_get_focus(pepper_touch_t *touch)
{
    /* TODO: */
    return NULL;
}

    PEPPER_API void
pepper_touch_send_down(pepper_touch_t *touch, uint32_t time, uint32_t id, double x, double y)
{
    /* TODO: wl_touch_send_down(resource, serial, time, touch->focus->surface->resource, x, y); */
}

    PEPPER_API void
pepper_touch_send_up(pepper_touch_t *touch, uint32_t time, uint32_t id)
{
    /* TODO: wl_touch_send_up(resource, serial, time, id); */
}

    PEPPER_API void
pepper_touch_send_motion(pepper_touch_t *touch, uint32_t time, uint32_t id, double x, double y)
{
    /* TODO: wl_touch_send_motion(resource, time, id, x, y); */
}

    PEPPER_API void
pepper_touch_send_frame(pepper_touch_t *touch)
{
    /* TODO: wl_touch_send_frame(resource); */
}

    PEPPER_API void
pepper_touch_send_cancel(pepper_touch_t *touch)
{
    /* TODO: wl_touch_send_cancel(resource); */
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
