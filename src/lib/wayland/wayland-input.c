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

#include "wayland-internal.h"
#include <stdlib.h>

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	/* TODO */
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
	/* TODO */
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	/* TODO */
}

static void
pointer_handle_button(void *data, struct wl_pointer *pointer,
                      uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	/* TODO */
}

static void
pointer_handle_axis(void *data, struct wl_pointer *pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
	/* TODO */
}

static const struct wl_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis,
};

static void
keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int32_t fd, uint32_t size)
{
	/* TODO */
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
	/* TODO */
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
	/* TODO */
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	/* TODO */
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
                          uint32_t mods_locked, uint32_t group)
{
	/* TODO */
}

static void
keyboard_handle_repeat_info(void *data, struct wl_keyboard *keyboard,
                            int32_t rate, int32_t delay)
{
	/* TODO */
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
	keyboard_handle_repeat_info
};

static void
touch_handle_down(void *data, struct wl_touch *touch,
                  uint32_t serial, uint32_t time, struct wl_surface *surface,
                  int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	/* TODO */
}

static void
touch_handle_up(void *data, struct wl_touch *touch,
                uint32_t serial, uint32_t time, int32_t id)
{
	/* TODO */
}

static void
touch_handle_motion(void *data, struct wl_touch *touch,
                    uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	/* TODO */
}

static void
touch_handle_frame(void *data, struct wl_touch *touch)
{
	/* TODO */
}

static void
touch_handle_cancel(void *data, struct wl_touch *touch)
{
	/* TODO */
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel,
};

static void
seat_handle_caps(void *data, struct wl_seat *s, enum wl_seat_capability caps)
{
	wayland_seat_t  *seat = (wayland_seat_t *)data;

	if (seat->seat != s) /* FIXME */
		return;

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && (!seat->pointer.wl_pointer)) {
		seat->pointer.wl_pointer = wl_seat_get_pointer(seat->seat);
		if (seat->pointer.wl_pointer)
			wl_pointer_add_listener(seat->pointer.wl_pointer, &pointer_listener, seat);

		seat->pointer.base = pepper_input_device_create(seat->conn->pepper,
		                     WL_SEAT_CAPABILITY_POINTER,
		                     NULL, NULL);
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && (seat->pointer.wl_pointer)) {
		pepper_input_device_destroy(seat->pointer.base);
		wl_pointer_release(seat->pointer.wl_pointer);
		seat->pointer.wl_pointer = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && (!seat->keyboard.wl_keyboard)) {
		seat->keyboard.wl_keyboard = wl_seat_get_keyboard(seat->seat);
		if (seat->keyboard.wl_keyboard)
			wl_keyboard_add_listener(seat->keyboard.wl_keyboard, &keyboard_listener, seat);

		seat->keyboard.base = pepper_input_device_create(seat->conn->pepper,
		                      WL_SEAT_CAPABILITY_KEYBOARD,
		                      NULL, NULL);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) &&
	           (seat->keyboard.wl_keyboard)) {
		pepper_input_device_destroy(seat->keyboard.base);
		wl_keyboard_release(seat->keyboard.wl_keyboard);
		seat->keyboard.wl_keyboard = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_TOUCH) && (!seat->touch.wl_touch)) {
		seat->touch.wl_touch = wl_seat_get_touch(seat->seat);
		if (seat->touch.wl_touch)
			wl_touch_add_listener(seat->touch.wl_touch, &touch_listener, seat);

		seat->touch.base = pepper_input_device_create(seat->conn->pepper,
		                   WL_SEAT_CAPABILITY_KEYBOARD,
		                   NULL, NULL);
	} else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && (seat->touch.wl_touch)) {
		pepper_input_device_destroy(seat->touch.base);
		wl_touch_release(seat->touch.wl_touch);
		seat->touch.wl_touch = NULL;
	}
}

static void
seat_handle_name(void *data, struct wl_seat *s, const char *name)
{
	/* TODO */
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_caps,
	seat_handle_name,
};

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version)
{
	wayland_seat_t  *seat;

	seat = (wayland_seat_t *)calloc(1, sizeof(wayland_seat_t));
	if (!seat) {
		PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
		return;
	}

	seat->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
	wl_seat_add_listener(seat->seat, &seat_listener, seat);

	seat->conn = conn;
	seat->id = name;

	pepper_list_insert(&conn->seat_list, &seat->link);
}
