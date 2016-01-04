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
touch_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_touch_interface touch_impl =
{
    touch_release,
};

static pepper_touch_point_t *
get_touch_point(pepper_touch_t *touch, uint32_t id)
{
    pepper_touch_point_t *point;

    pepper_list_for_each(point, &touch->point_list, link)
    {
        if (point->id == id)
            return point;
    }

    return NULL;
}

static void
touch_point_set_focus(pepper_touch_point_t *point, pepper_view_t *focus);

static void
touch_point_handle_focus_destroy(pepper_event_listener_t *listener, pepper_object_t *surface,
                                 uint32_t id, void *info, void *data)
{
    pepper_touch_point_t *point = data;
    touch_point_set_focus(point, NULL);
}

static void
touch_point_set_focus(pepper_touch_point_t *point, pepper_view_t *focus)
{
    if (point->focus == focus)
        return;

    if (point->focus)
    {
        pepper_event_listener_remove(point->focus_destroy_listener);
        pepper_object_emit_event(&point->touch->base, PEPPER_EVENT_FOCUS_LEAVE, point->focus);
        pepper_object_emit_event(&point->focus->base, PEPPER_EVENT_FOCUS_LEAVE, point->focus);
    }

    point->focus = focus;

    if (focus)
    {
        point->focus_serial = wl_display_next_serial(point->touch->seat->compositor->display);

        point->focus_destroy_listener =
            pepper_object_add_event_listener(&focus->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                            touch_point_handle_focus_destroy, point);

        pepper_object_emit_event(&point->touch->base, PEPPER_EVENT_FOCUS_ENTER, focus);
        pepper_object_emit_event(&focus->base, PEPPER_EVENT_FOCUS_ENTER, focus);
    }
}

void
pepper_touch_handle_event(pepper_touch_t *touch, uint32_t id, pepper_input_event_t *event)
{
    switch (id)
    {
    case PEPPER_EVENT_TOUCH_DOWN:
        {
            if (touch->grab)
                touch->grab->down(touch, touch->data, event->time, event->slot, event->x, event->y);
        }
        break;
    case PEPPER_EVENT_TOUCH_UP:
        {
            if (touch->grab)
                touch->grab->up(touch, touch->data, event->time, event->slot);
        }
        break;
    case PEPPER_EVENT_TOUCH_MOTION:
        {
            pepper_touch_point_t *point = get_touch_point(touch, event->slot);

            point->x = event->x;
            point->y = event->y;

            if (touch->grab)
                touch->grab->motion(touch, touch->data, event->time, event->slot, event->x, event->y);
        }
        break;
    case PEPPER_EVENT_TOUCH_FRAME:
        {
            if (touch->grab)
                touch->grab->frame(touch, touch->data);
        }
        break;
    case PEPPER_EVENT_TOUCH_CANCEL:
        {
            if (touch->grab)
                touch->grab->cancel(touch, touch->data);
        }
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
    pepper_list_init(&touch->point_list);

    return touch;
}

void
pepper_touch_destroy(pepper_touch_t *touch)
{
    pepper_touch_point_t *point, *tmp;

    PEPPER_CHECK(touch, return, "pepper_touch_destroy() failed.\n");

    pepper_list_for_each_safe(point, tmp, &touch->point_list, link)
    {
        if (point->focus_destroy_listener)
            pepper_event_listener_remove(point->focus_destroy_listener);

        free(point);
    }

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

/**
 * Get the list of wl_resource of the given touch
 *
 * @param touch touch object
 *
 * @return list of the touch resources
 */
PEPPER_API struct wl_list *
pepper_touch_get_resource_list(pepper_touch_t *touch)
{
    return &touch->resource_list;
}

/**
 * Get the compositor of the given touch
 *
 * @param touch touch object
 *
 * @return compositor
 */
PEPPER_API pepper_compositor_t *
pepper_touch_get_compositor(pepper_touch_t *touch)
{
    return touch->seat->compositor;
}

/**
 * Get the seat of the given touch
 *
 * @param touch touch object
 *
 * @return seat
 */
PEPPER_API pepper_seat_t *
pepper_touch_get_seat(pepper_touch_t *touch)
{
    return touch->seat;
}

/**
 * Add a touch pointer to the given touch
 *
 * @param touch touch object
 * @param id    touch point id
 * @param x     x coordinate in global space
 * @param y     y coordinate in global space
 */
PEPPER_API void
pepper_touch_add_point(pepper_touch_t *touch, uint32_t id, double x, double y)
{
    pepper_touch_point_t *point = get_touch_point(touch, id);
    PEPPER_CHECK(!point, return, "Touch point %d already exist.\n", id);

    point = calloc(1, sizeof(pepper_touch_point_t));
    PEPPER_CHECK(point, return, "malloc() failed.\n");

    point->touch = touch;
    point->id = id;
    point->x = x;
    point->y = y;

    pepper_list_insert(touch->point_list.prev, &point->link);
}

/**
 * Remove a touch point from the given touch
 *
 * @param touch touch object
 * @param id    touch point id
 */
PEPPER_API void
pepper_touch_remove_point(pepper_touch_t *touch, uint32_t id)
{
    pepper_touch_point_t *point = get_touch_point(touch, id);
    PEPPER_CHECK(point, return, "Touch point %d does not exist.\n", id);

    touch_point_set_focus(point, NULL);
    pepper_list_remove(&point->link);
    free(point);
}

/**
 * Set the focus view of the given touch
 *
 * @param touch touch object
 * @param focus focus view
 *
 * @see pepper_touch_send_enter()
 * @see pepper_touch_send_leave()
 * @see pepper_touch_get_focus()
 */
PEPPER_API void
pepper_touch_set_focus(pepper_touch_t *touch, uint32_t id, pepper_view_t *focus)
{
    pepper_touch_point_t *point = get_touch_point(touch, id);

    if (focus)
    {
        if (!point)
            pepper_touch_add_point(touch, id, 0, 0);

        touch_point_set_focus(get_touch_point(touch, id), focus);
    }
    else
    {
        if (point)
            pepper_touch_remove_point(touch, id);
    }
}

/**
 * Get the focus view of the given touch
 *
 * @param touch touch object
 *
 * @return focus view
 *
 * @see pepper_touch_set_focus()
 */
PEPPER_API pepper_view_t *
pepper_touch_get_focus(pepper_touch_t *touch, uint32_t id)
{
    pepper_touch_point_t *point = get_touch_point(touch, id);
    PEPPER_CHECK(point, return NULL, "Touch point %d does not exist.\n", id);

    return point->focus;
}

/**
 * Get the position of the given touch point
 *
 * @param touch touch object
 * @param id    touch point id
 * @param x     pointer to receive x coordinate in global space
 * @param y     pointer to receive y coordinate in global space
 *
 * @return PEPPER_TRUE on success, PEPPER_FALSE otherwise
 */
PEPPER_API pepper_bool_t
pepper_touch_get_position(pepper_touch_t *touch, uint32_t id, double *x, double *y)
{
    pepper_touch_point_t *point = get_touch_point(touch, id);
    PEPPER_CHECK(point, return PEPPER_FALSE, "Touch point %d does not exist.\n", id);

    if (x)
        *x = point->x;

    if (y)
        *y = point->y;

    return PEPPER_TRUE;
}

/**
 * Set the position of the given touch point
 *
 * @param touch touch object
 * @param id    touch point id
 * @param x     x coordinate in global space
 * @param y     y coordinate in global space
 *
 * @return PEPPER_TRUE on success, PEPPER_FALSE otherwise
 */
PEPPER_API pepper_bool_t
pepper_touch_set_position(pepper_touch_t *touch, uint32_t id, double x, double y)
{
    pepper_touch_point_t *point = get_touch_point(touch, id);
    PEPPER_CHECK(point, return PEPPER_FALSE, "Touch point %d does not exist.\n", id);

    point->x = x;
    point->y = y;

    return PEPPER_TRUE;
}

/**
 * Send wl_touch.down event to the client
 *
 * @param touch touch object
 * @param view  target view
 * @param time  time in mili-second with undefined base
 * @param id    touch point id
 * @param x     x coordinate in global space
 * @param y     y coordinate in global space
 */
PEPPER_API void
pepper_touch_send_down(pepper_touch_t *touch, pepper_view_t *view,
                       uint32_t time, uint32_t id, double x, double y)
{
    struct wl_resource     *resource;
    wl_fixed_t              fx = wl_fixed_from_double(x);
    wl_fixed_t              fy = wl_fixed_from_double(y);
    pepper_touch_point_t   *point = get_touch_point(touch, id);
    pepper_input_event_t    event;

    if (!point || !view || !view->surface || !view->surface->resource)
        return;

    wl_resource_for_each(resource, &touch->resource_list)
    {
        struct wl_resource *surface_resource = view->surface->resource;

        if (wl_resource_get_client(resource) == wl_resource_get_client(surface_resource))
            wl_touch_send_down(resource, point->focus_serial, time, surface_resource, id, fx, fy);
    }

    event.time = time;
    event.x = x;
    event.y = y;
    pepper_object_emit_event(&view->base, PEPPER_EVENT_TOUCH_DOWN, &event);
}

/**
 * Send wl_touch.up event to the client
 *
 * @param touch touch object
 * @param view  target view
 * @param time  time in mili-second with undefined base
 * @param id    touch point id
 */
PEPPER_API void
pepper_touch_send_up(pepper_touch_t *touch, pepper_view_t *view, uint32_t time, uint32_t id)
{
    struct wl_resource     *resource;
    uint32_t                serial;
    pepper_touch_point_t   *point = get_touch_point(touch, id);
    pepper_input_event_t    event;

    if (!point || !view || !view->surface || !view->surface->resource)
        return;

    serial = wl_display_next_serial(touch->seat->compositor->display);

    wl_resource_for_each(resource, &touch->resource_list)
    {
        struct wl_resource *surface_resource = view->surface->resource;

        if (wl_resource_get_client(resource) == wl_resource_get_client(surface_resource))
            wl_touch_send_up(resource, serial, time, id);
    }

    event.time = time;
    pepper_object_emit_event(&view->base, PEPPER_EVENT_TOUCH_UP, &event);
}

/**
 * Send wl_touch.motion event to the client
 *
 * @param touch touch object
 * @param view  target view
 * @param time  time in mili-second with undefined base
 * @param id    touch point id
 * @param x     x coordinate in global space
 * @param y     y coordinate in global space
 */
PEPPER_API void
pepper_touch_send_motion(pepper_touch_t *touch, pepper_view_t *view, uint32_t time, uint32_t id, double x, double y)
{

    struct wl_resource     *resource;
    wl_fixed_t              fx = wl_fixed_from_double(x);
    wl_fixed_t              fy = wl_fixed_from_double(y);
    pepper_touch_point_t   *point = get_touch_point(touch, id);
    pepper_input_event_t    event;

    if (!point || !view || !view->surface || !view->surface->resource)
        return;

    wl_resource_for_each(resource, &touch->resource_list)
    {
        struct wl_resource *surface_resource = view->surface->resource;

        if (wl_resource_get_client(resource) == wl_resource_get_client(surface_resource))
            wl_touch_send_motion(resource, time, id, fx, fy);
    }

    event.time = time;
    event.x = x;
    event.y = y;
    pepper_object_emit_event(&view->base, PEPPER_EVENT_TOUCH_MOTION, &event);
}

/**
 * Send wl_touch.frame event to the client
 *
 * @param touch touch object
 * @param view  target view
 */
PEPPER_API void
pepper_touch_send_frame(pepper_touch_t *touch, pepper_view_t *view)
{
    struct wl_resource     *resource;

    wl_resource_for_each(resource, &touch->resource_list)
        wl_touch_send_frame(resource);

    pepper_object_emit_event(&view->base, PEPPER_EVENT_TOUCH_FRAME, NULL);
}

/**
 * Send wl_touch.cancel event to the client
 *
 * @param touch touch object
 * @param view  target view
 */
PEPPER_API void
pepper_touch_send_cancel(pepper_touch_t *touch, pepper_view_t *view)
{
    struct wl_resource     *resource;

    wl_resource_for_each(resource, &touch->resource_list)
        wl_touch_send_cancel(resource);

    pepper_object_emit_event(&view->base, PEPPER_EVENT_TOUCH_CANCEL, NULL);
}

/**
 * Install touch grab
 *
 * @param touch touch object
 * @param grab  grab handler
 * @param data  user data to be passed to grab functions
 */
PEPPER_API void
pepper_touch_set_grab(pepper_touch_t *touch, const pepper_touch_grab_t *grab, void *data)
{
    touch->grab = grab;
    touch->data = data;
}

/**
 * Get the current touch grab
 *
 * @param touch touch object
 *
 * @return grab handler which is most recently installed
 *
 * @see pepper_touch_set_grab()
 * @see pepper_touch_get_grab_data()
 */
PEPPER_API const pepper_touch_grab_t *
pepper_touch_get_grab(pepper_touch_t *touch)
{
    return touch->grab;
}

/**
 * Get the current touch grab data
 *
 * @param touch touch object
 *
 * @return grab data which is most recently installed
 *
 * @see pepper_touch_set_grab()
 * @see pepper_touch_get_grab()
 */
PEPPER_API void *
pepper_touch_get_grab_data(pepper_touch_t *touch)
{
    return touch->data;
}
