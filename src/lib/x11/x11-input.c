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

#include "x11-internal.h"
#include <stdlib.h>
#include <linux/input.h>

#define UNUSED(x)   (void)(x)

static inline uint32_t
get_standard_button(uint32_t xcb_button)
{
	switch (xcb_button) {
	case XCB_BUTTON_INDEX_1:
		return BTN_LEFT;
	case XCB_BUTTON_INDEX_2:
		return BTN_MIDDLE;
	case XCB_BUTTON_INDEX_3:
		return BTN_RIGHT;
	case XCB_BUTTON_INDEX_4:
	case XCB_BUTTON_INDEX_5:
		break;
	}

	return 0;
}

void
x11_handle_input_event(x11_seat_t *seat, uint32_t type,
		       xcb_generic_event_t *xev)
{
	pepper_input_event_t event;

	switch (type) {
	case XCB_ENTER_NOTIFY: {
		PEPPER_TRACE("enter\n");
	}
	break;
	case XCB_LEAVE_NOTIFY: {
		PEPPER_TRACE("leave\n");
	}
	break;
	case XCB_KEY_PRESS: {
		xcb_key_press_event_t *kp = (xcb_key_press_event_t *)xev;

		event.time  = kp->time;
		event.key   = kp->detail - 8;
		event.state = PEPPER_KEY_STATE_PRESSED;

		pepper_object_emit_event((pepper_object_t *)seat->keyboard,
					 PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY, &event);
	}
	break;
	case XCB_KEY_RELEASE: {
		xcb_key_release_event_t *kr = (xcb_key_release_event_t *)xev;

		event.time  = kr->time;
		event.key   = kr->detail - 8;
		event.state = PEPPER_KEY_STATE_RELEASED;

		pepper_object_emit_event((pepper_object_t *)seat->keyboard,
					 PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY, &event);
	}
	break;
	case XCB_BUTTON_PRESS: {
		xcb_button_press_event_t *bp = (xcb_button_press_event_t *)xev;

		event.time   = bp->time;
		event.state  = PEPPER_BUTTON_STATE_PRESSED;
		event.button = get_standard_button(bp->detail);

		pepper_object_emit_event((pepper_object_t *)seat->pointer,
					 PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON, &event);
	}
	break;
	case XCB_BUTTON_RELEASE: {
		xcb_button_release_event_t *br = (xcb_button_release_event_t *)xev;

		event.time   = br->time;
		event.state  = PEPPER_BUTTON_STATE_RELEASED;
		event.button = get_standard_button(br->detail);

		pepper_object_emit_event((pepper_object_t *)seat->pointer,
					 PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON, &event);
	}
	break;
	case XCB_MOTION_NOTIFY: {
		xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)xev;

		event.time = motion->time;
		event.x    = motion->event_x;
		event.y    = motion->event_y;

		pepper_object_emit_event((pepper_object_t *)seat->pointer,
					 PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE, &event);
	}
	break;
	default :
		PEPPER_ERROR("unknown input event, [0x%x]\n", type);
		break;
	}
}

void
x11_window_input_property_change(xcb_connection_t *conn, xcb_window_t window)
{
	static const uint32_t values[] = {
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_KEY_PRESS |
		XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_ENTER_WINDOW |
		XCB_EVENT_MASK_LEAVE_WINDOW |
		XCB_EVENT_MASK_KEYMAP_STATE |
		XCB_EVENT_MASK_FOCUS_CHANGE
	};
	xcb_change_window_attributes(conn, window, XCB_CW_EVENT_MASK, values);
	xcb_flush(conn);
}

void
x11_seat_destroy(void *data)
{
	x11_seat_t *seat = (x11_seat_t *)data;

	if (seat->pointer)
		pepper_input_device_destroy(seat->pointer);

	if (seat->keyboard)
		pepper_input_device_destroy(seat->keyboard);

	if (seat->conn)
		seat->conn->seat = NULL;

	free(seat);
}

PEPPER_API pepper_bool_t
pepper_x11_input_create(pepper_x11_connection_t *conn)
{
	x11_seat_t      *seat;
	x11_output_t    *out, *tmp;

	if (!conn) {
		PEPPER_ERROR("connection is null...\n");
		return PEPPER_FALSE;
	}

	seat = calloc(1, sizeof(x11_seat_t));
	if (!seat) {
		PEPPER_ERROR("failed to allocate memory\n");
		return PEPPER_FALSE;
	}

	conn->use_xinput = PEPPER_TRUE;

	pepper_list_for_each_safe(out, tmp, &conn->output_list, link)
	x11_window_input_property_change(conn->xcb_connection, out->window);

	/* XXX: if x-input-module used without x-output-module,
	 * need to create dummy window for input with output-size */

	seat->id = X11_BACKEND_INPUT_ID;

	/* Hard-coded: */
	seat->pointer = pepper_input_device_create(conn->compositor,
			WL_SEAT_CAPABILITY_POINTER,
			NULL, NULL);
	if (!seat->pointer) {
		PEPPER_ERROR("failed to create pepper pointer device\n");

		goto failed;
	}
	seat->caps |= WL_SEAT_CAPABILITY_POINTER;

	seat->keyboard = pepper_input_device_create(conn->compositor,
			 WL_SEAT_CAPABILITY_KEYBOARD,
			 NULL, NULL);
	if (!seat->keyboard) {
		PEPPER_ERROR("failed to create pepper keyboard device\n");

		goto failed;
	}
	seat->caps |= WL_SEAT_CAPABILITY_KEYBOARD;

	/* x-connection has only 1 seat */
	conn->seat = seat;
	seat->conn = conn;

	return PEPPER_TRUE;

failed:
	x11_seat_destroy(seat);
	return PEPPER_FALSE;
}
