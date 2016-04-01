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
#include <float.h>

static pepper_view_t *
get_cursor_view(pepper_pointer_t *pointer)
{
	if (pointer->cursor_view)
		return pointer->cursor_view;

	pointer->cursor_view = pepper_compositor_add_view(pointer->seat->compositor);

	return pointer->cursor_view;
}

static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource,
				   uint32_t serial,
				   struct wl_resource *surface_resource, int32_t x, int32_t y)
{
	pepper_pointer_t   *pointer = (pepper_pointer_t *)wl_resource_get_user_data(
									  resource);
	pepper_surface_t   *surface;
	pepper_view_t      *cursor_view;

	cursor_view = get_cursor_view(pointer);
	PEPPER_CHECK(cursor_view, return, "failed to get cursor view\n");

	if (!surface_resource) {
		pepper_view_set_surface(cursor_view, NULL);
		return;
	}

	surface = (pepper_surface_t *)wl_resource_get_user_data(surface_resource);
	if (!surface->role)
		pepper_surface_set_role(surface, "wl_pointer-cursor");
	else if (strcmp("wl_pointer-cursor", surface->role))
		return;

	if (surface != pepper_view_get_surface(cursor_view)) {
		surface->pickable = PEPPER_FALSE;
		pixman_region32_fini(&surface->input_region);
		pixman_region32_init(&surface->input_region);
		pepper_view_set_surface(cursor_view, surface);
	}

	pointer->hotspot_x = x;
	pointer->hotspot_y = y;
	pepper_view_set_position(cursor_view, pointer->x - x, pointer->y - y);
	pepper_view_map(cursor_view);
}

static void
pointer_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_pointer_interface pointer_impl = {
	pointer_set_cursor,
	pointer_release,
};

static pepper_bool_t
pointer_clamp(pepper_pointer_t *pointer)
{
	pepper_bool_t clamped = PEPPER_FALSE;

	if (pointer->x < pointer->clamp.x0) {
		pointer->x = pointer->clamp.x0;
		clamped = PEPPER_TRUE;
	}

	if (pointer->x > pointer->clamp.x1) {
		pointer->x = pointer->clamp.x1;
		clamped = PEPPER_TRUE;
	}

	if (pointer->y < pointer->clamp.y0) {
		pointer->y = pointer->clamp.y0;
		clamped = PEPPER_TRUE;
	}

	if (pointer->y > pointer->clamp.y1) {
		pointer->y = pointer->clamp.y1;
		clamped = PEPPER_TRUE;
	}

	return clamped;
}

static void
pointer_set_position(pepper_pointer_t *pointer, uint32_t time, double x,
					 double y)
{
	pepper_input_event_t event;

	if (x == pointer->x && y == pointer->y)
		return;

	pointer->x = x;
	pointer->y = y;

	pointer_clamp(pointer);

	if (pointer->cursor_view)
		pepper_view_set_position(pointer->cursor_view,
								 x - pointer->hotspot_x, y - pointer->hotspot_y);

	if (pointer->grab)
		pointer->grab->motion(pointer, pointer->data, time, pointer->x, pointer->y);

	/* Emit motion event. */
	memset(&event, 0x00, sizeof(pepper_input_event_t));

	event.time = time;
	event.x = pointer->x;
	event.y = pointer->y;
	pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_MOTION, &event);
}

void
pepper_pointer_handle_event(pepper_pointer_t *pointer, uint32_t id,
							pepper_input_event_t *event)
{
	pointer->time = event->time;

	switch (id) {
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE: {
		pointer_set_position(pointer, event->time, event->x, event->y);
	}
	break;
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION: {
		pointer_set_position(pointer, event->time,
							 pointer->x + event->x * pointer->x_velocity,
							 pointer->y + event->y * pointer->y_velocity);
	}
	break;
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON: {
		if (pointer->grab) {
			pointer->grab->button(pointer, pointer->data,
								  event->time, event->button, event->state);
		}

		pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_BUTTON, event);
	}
	break;
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_AXIS: {
		if (pointer->grab)
			pointer->grab->axis(pointer, pointer->data, event->time, event->axis,
								event->value);

		pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_AXIS, event);
	}
	break;
	}
}

static void
pointer_handle_focus_destroy(pepper_event_listener_t *listener,
							 pepper_object_t *surface,
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
		(pepper_pointer_t *)pepper_object_alloc(PEPPER_OBJECT_POINTER,
				sizeof(pepper_pointer_t));

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
pepper_pointer_bind_resource(struct wl_client *client,
							 struct wl_resource *resource, uint32_t id)
{
	pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
	pepper_pointer_t   *pointer = seat->pointer;
	struct wl_resource *res;

	if (!pointer)
		return;

	res = wl_resource_create(client, &wl_pointer_interface,
							 wl_resource_get_version(resource), id);
	if (!res) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&pointer->resource_list, wl_resource_get_link(res));
	wl_resource_set_implementation(res, &pointer_impl, pointer, unbind_resource);

	if (!pointer->focus || !pointer->focus->surface ||
		!pointer->focus->surface->resource)
		return;

	if (wl_resource_get_client(pointer->focus->surface->resource) == client) {
		wl_pointer_send_enter(res, pointer->focus_serial,
							  pointer->focus->surface->resource,
							  wl_fixed_from_double(pointer->vx), wl_fixed_from_double(pointer->vy));
	}
}

/**
 * Get the list of wl_resource of the given pointer
 *
 * @param pointer   pointer object
 *
 * @return list of the pointer resources
 */
PEPPER_API struct wl_list *
pepper_pointer_get_resource_list(pepper_pointer_t *pointer)
{
	return &pointer->resource_list;
}

/**
 * Get the compositor of the given pointer
 *
 * @param pointer   pointer object
 *
 * @return compositor
 */
PEPPER_API pepper_compositor_t *
pepper_pointer_get_compositor(pepper_pointer_t *pointer)
{
	return pointer->seat->compositor;
}

/**
 * Get the seat of the given pointer
 *
 * @param pointer   pointer object
 *
 * @return seat
 */
PEPPER_API pepper_seat_t *
pepper_pointer_get_seat(pepper_pointer_t *pointer)
{
	return pointer->seat;
}

/**
 * Set the clamp area of the given pointer
 *
 * @param pointer   pointer object
 * @param x0        x coordinate of top left corner of the clamp area
 * @param y0        y coordinate of top left corner of the clamp area
 * @param x1        x coordinate of bottom right corner of the clamp area
 * @param y1        y coordinate of bottom right corner of the clamp area
 *
 * @return PEPPER_TRUE on success, PEPPER_FALSE otherwise
 *
 * Clamp area is a rectangular boundary that a pointer's cursor can never go out of it. Changing the
 * clamp area might cause the pointer cursor to be moved (to be located inside of the area)
 * resulting motion events.
 */
PEPPER_API pepper_bool_t
pepper_pointer_set_clamp(pepper_pointer_t *pointer, double x0, double y0,
						 double x1, double y1)
{
	if (x1 < x0 || y1 < y0)
		return PEPPER_FALSE;

	pointer->clamp.x0 = x0;
	pointer->clamp.y0 = y0;
	pointer->clamp.x1 = x1;
	pointer->clamp.y1 = y1;

	if (pointer_clamp(pointer)) {
		pepper_input_event_t event;

		if (pointer->grab)
			pointer->grab->motion(pointer, pointer->data, pointer->time, pointer->x,
								  pointer->y);

		memset(&event, 0x00, sizeof(pepper_input_event_t));

		event.time = pointer->time;
		event.x = pointer->x;
		event.y = pointer->y;
		pepper_object_emit_event(&pointer->base, PEPPER_EVENT_POINTER_MOTION, &event);
	}

	return PEPPER_TRUE;
}

/**
 * Get the clamp area of the given pointer
 *
 * @param pointer   pointer object
 * @param x0        pointer to receive x coordinate of top left corner of the clamp area
 * @param y0        pointer to receive y coordinate of top left corner of the clamp area
 * @param x1        pointer to receive x coordinate of bottom right corner of the clamp area
 * @param y1        pointer to receive y coordinate of bottom right corner of the clamp area
 */
PEPPER_API void
pepper_pointer_get_clamp(pepper_pointer_t *pointer, double *x0, double *y0,
						 double *x1, double *y1)
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

/**
 * Set the velocity of the given pointer
 *
 * @param pointer   pointer object
 * @param vx        velocity in x direction
 * @param vy        velocity in y direction
 *
 * Pointer cursor movement is calculated as follows.
 *
 *  dx = x_velocity * motion.x
 *  dy = y_velocity * motion.x
 *
 * Default value is (1.0, 1.0).
 */
PEPPER_API void
pepper_pointer_set_velocity(pepper_pointer_t *pointer, double vx, double vy)
{
	pointer->x_velocity = vx;
	pointer->y_velocity = vy;
}

/**
 * Get the velocity of the given pointer
 *
 * @param pointer   pointer object
 * @param vx        pointer to receive velocity in x direction
 * @param vx        pointer to receive velocity in y direction
 *
 * @see pepper_pointer_set_velocity()
 */
PEPPER_API void
pepper_pointer_get_velocity(pepper_pointer_t *pointer, double *vx, double *vy)
{
	if (vx)
		*vx = pointer->x_velocity;

	if (vy)
		*vy = pointer->y_velocity;
}

/**
 * Get the current cursor position of the given pointer
 *
 * @param pointer   pointer object
 * @param x         pointer to receive x coordinate of the cursor in global space
 * @param x         pointer to receive y coordinate of the cursor in global space
 */
PEPPER_API void
pepper_pointer_get_position(pepper_pointer_t *pointer, double *x, double *y)
{
	if (x)
		*x = pointer->x;

	if (y)
		*y = pointer->y;
}

/**
 * Set the focus view of the given pointer
 *
 * @param pointer   pointer object
 * @param focus     focus view
 *
 * @see pepper_pointer_send_enter()
 * @see pepper_pointer_send_leave()
 * @see pepper_pointer_get_focus()
 */
PEPPER_API void
pepper_pointer_set_focus(pepper_pointer_t *pointer, pepper_view_t *focus)
{
	if (pointer->focus == focus)
		return;

	if (pointer->focus) {
		pepper_event_listener_remove(pointer->focus_destroy_listener);
		pepper_object_emit_event(&pointer->base, PEPPER_EVENT_FOCUS_LEAVE,
								 pointer->focus);
		pepper_object_emit_event(&pointer->focus->base, PEPPER_EVENT_FOCUS_LEAVE,
								 pointer);
	}

	pointer->focus = focus;

	if (focus) {
		pointer->focus_serial = wl_display_next_serial(
									pointer->seat->compositor->display);

		pointer->focus_destroy_listener =
			pepper_object_add_event_listener(&focus->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
											 pointer_handle_focus_destroy, pointer);

		pepper_object_emit_event(&pointer->base, PEPPER_EVENT_FOCUS_ENTER, focus);
		pepper_object_emit_event(&focus->base, PEPPER_EVENT_FOCUS_ENTER, pointer);
	}
}

/**
 * Get the focus view of the given pointer
 *
 * @param pointer   pointer object
 *
 * @return focus view
 *
 * @see pepper_pointer_set_focus()
 */
PEPPER_API pepper_view_t *
pepper_pointer_get_focus(pepper_pointer_t *pointer)
{
	return pointer->focus;
}

/**
 * Send wl_pointer.leave event to the client
 *
 * @param pointer   pointer object
 * @param view      view object having the target surface for the leave event
 */
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

	wl_resource_for_each(resource, &pointer->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_pointer_send_leave(resource, serial, view->surface->resource);
	}
}

/**
 * Send wl_pointer.enter event to the client
 *
 * @param pointer   pointer object
 * @param view      view object having the target surface for the enter event
 */
PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, pepper_view_t *view,
						  double x, double y)
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

	wl_resource_for_each(resource, &pointer->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_pointer_send_enter(resource, serial, view->surface->resource, fx, fy);
	}
}

/**
 * Send wl_pointer.motion event to the client
 *
 * @param pointer   pointer object
 * @param view      view object having the target surface for the motion event
 * @param time      time in mili-second with undefined base
 * @param x         movement in x direction in global space
 * @param y         movement in y direction in global space
 */
PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, pepper_view_t *view,
						   uint32_t time, double x, double y)
{
	struct wl_resource     *resource;
	wl_fixed_t              fx = wl_fixed_from_double(x);
	wl_fixed_t              fy = wl_fixed_from_double(y);
	struct wl_client       *client;
	pepper_input_event_t    event;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);

	wl_resource_for_each(resource, &pointer->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_pointer_send_motion(resource, time, fx, fy);
	}

	event.time = time;
	event.x = x;
	event.y = y;
	pepper_object_emit_event(&view->base, PEPPER_EVENT_POINTER_MOTION, &event);
}

/**
 * Send wl_pointer.button event to the client
 *
 * @param pointer   pointer object
 * @param view      view object having the target surface for the button event
 * @param time      time in mili-second with undefined base
 * @param button    button flag (ex. BTN_LEFT)
 * @param state     state flag (ex. WL_POINTER_BUTTON_STATE_PRESSED)
 */
PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, pepper_view_t *view,
						   uint32_t time, uint32_t button, uint32_t state)
{
	struct wl_resource     *resource;
	struct wl_client       *client;
	uint32_t                serial;
	pepper_input_event_t    event;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(pointer->seat->compositor->display);

	wl_resource_for_each(resource, &pointer->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_pointer_send_button(resource, serial, time, button, state);
	}

	event.time = time;
	event.button = button;
	event.state = state;
	pepper_object_emit_event(&view->base, PEPPER_EVENT_POINTER_BUTTON, &event);
}

/**
 * Send wl_pointer.axis event to the client
 *
 * @param pointer   pointer object
 * @param view      view object having the target surface for the axis event
 * @param time      time in mili-second with undefined base
 * @param axis      axis flag (ex. WL_POINTER_AXIS_VERTICAL_SCROLL)
 * @param value     amount of the scrolling
 */
PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, pepper_view_t *view,
						 uint32_t time, uint32_t axis, double value)
{
	struct wl_resource     *resource;
	wl_fixed_t              v = wl_fixed_from_double(value);
	struct wl_client       *client;
	pepper_input_event_t    event;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);

	wl_resource_for_each(resource, &pointer->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_pointer_send_axis(resource, time, axis, v);
	}

	event.time = time;
	event.axis = axis;
	event.value = value;
	pepper_object_emit_event(&view->base, PEPPER_EVENT_POINTER_AXIS, &event);
}

/**
 * Install pointer grab
 *
 * @param pointer   pointer object
 * @param grab      grab handler
 * @param data      user data to be passed to grab functions
 */
PEPPER_API void
pepper_pointer_set_grab(pepper_pointer_t *pointer,
						const pepper_pointer_grab_t *grab, void *data)
{
	pointer->grab = grab;
	pointer->data = data;
}

/**
 * Get the current pointer grab
 *
 * @param pointer   pointer object
 *
 * @return grab handler which is most recently installed
 *
 * @see pepper_pointer_set_grab()
 * @see pepper_pointer_get_grab_data()
 */
PEPPER_API const pepper_pointer_grab_t *
pepper_pointer_get_grab(pepper_pointer_t *pointer)
{
	return pointer->grab;
}

/**
 * Get the current pointer grab data
 *
 * @param pointer   pointer object
 *
 * @return grab data which is most recently installed
 *
 * @see pepper_pointer_set_grab()
 * @see pepper_pointer_get_grab()
 */
PEPPER_API void *
pepper_pointer_get_grab_data(pepper_pointer_t *pointer)
{
	return pointer->data;
}

PEPPER_API pepper_view_t *
pepper_pointer_get_cursor_view(pepper_pointer_t *pointer)
{
	return pointer->cursor_view;
}

PEPPER_API void
pepper_pointer_set_hotspot(pepper_pointer_t *pointer, int32_t x, int32_t y)
{
	pointer->hotspot_x = x;
	pointer->hotspot_y = y;
}
