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

#ifndef X11_INTERNAL_H
#define X11_INTERNAL_H

#include "pepper-x11.h"

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <string.h>
#include <pixman.h>
#include <pepper-output-backend.h>
#include <pepper-input-backend.h>
#include <pepper-render.h>
#include <stdio.h>

#define X11_BACKEND_INPUT_ID    0x12345678

typedef struct x11_output       x11_output_t;
typedef struct x11_cursor       x11_cursor_t;
typedef struct x11_seat         x11_seat_t;
typedef struct x11_shm_image    x11_shm_image_t;

struct x11_shm_image {
	int                     shm_id;
	void                   *buf;
	pepper_format_t         format;
	int                     stride;
	int                     w, h;
	xcb_shm_seg_t           segment;
	pepper_render_target_t *target; /* XXX: need double-buffering? */
};

struct x11_output {
	pepper_output_t            *base;
	pepper_x11_connection_t    *connection;
	char                        name[32];
	pepper_list_t               link;

	int32_t                  x, y;
	int32_t                  w, h;
	uint32_t                 subpixel;
	uint8_t                  depth;
	uint8_t                  bpp;

	xcb_window_t             window;
	xcb_gc_t                 gc;
	x11_cursor_t            *cursor;

	pepper_renderer_t       *renderer;
	x11_shm_image_t          shm;
	pepper_render_target_t  *target;
	pepper_render_target_t  *gl_target;

	struct wl_event_source  *frame_done_timer;
	struct wl_listener       conn_destroy_listener;

	pepper_plane_t          *primary_plane;
};

struct x11_seat {
	pepper_x11_connection_t        *conn;

	pepper_input_device_t          *pointer;
	pepper_input_device_t          *keyboard;
	pepper_input_device_t          *touch;

	uint32_t                        id;
	uint32_t                        caps;
	char                           *name;

	pepper_list_t                   link;

	struct wl_listener              conn_destroy_listener;
};

struct pepper_x11_connection {
	pepper_compositor_t    *compositor;
	char                   *display_name;

	Display                *display;
	xcb_screen_t           *screen;
	xcb_connection_t       *xcb_connection;

	struct wl_event_source *xcb_event_source;
	int fd;

	pepper_list_t           output_list;

	pepper_bool_t           use_xinput;
	x11_seat_t             *seat;

	struct {
		xcb_atom_t          wm_protocols;
		xcb_atom_t          wm_normal_hints;
		xcb_atom_t          wm_size_hints;
		xcb_atom_t          wm_delete_window;
		xcb_atom_t          wm_class;
		xcb_atom_t          net_wm_name;
		xcb_atom_t          net_supporting_wm_check;
		xcb_atom_t          net_supported;
		xcb_atom_t          net_wm_icon;
		xcb_atom_t          net_wm_state;
		xcb_atom_t          net_wm_state_fullscreen;
		xcb_atom_t          string;
		xcb_atom_t          utf8_string;
		xcb_atom_t          cardinal;
		xcb_atom_t          xkb_names;
	} atom;

	pepper_renderer_t       *pixman_renderer;
	pepper_renderer_t       *gl_renderer;
};

struct x11_cursor {
	xcb_cursor_t     xcb_cursor;
	int              w, h;
	uint8_t         *data;
};

/* it declared in xcb-icccm.h */
typedef struct xcb_size_hints {
	uint32_t flags;
	uint32_t pad[4];
	int32_t  min_width, min_height;
	int32_t  max_width, max_height;
	int32_t  width_inc, height_inc;
	int32_t  min_aspect_x, min_aspect_y;
	int32_t  max_aspect_x, max_aspect_y;
	int32_t  base_width, base_height;
	int32_t  win_gravity;
} xcb_size_hints_t;
#define WM_NORMAL_HINTS_MIN_SIZE        16
#define WM_NORMAL_HINTS_MAX_SIZE        32
/* -- xcb-icccm.h */

void
x11_window_input_property_change(xcb_connection_t *conn, xcb_window_t window);

void
x11_handle_input_event(x11_seat_t *seat, uint32_t type,
                       xcb_generic_event_t *xev);

void
x11_output_destroy(void *o);

void
x11_seat_destroy(void *data);

#endif  /*X11_INTERNAL_H*/
