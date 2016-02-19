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

#include <config.h>
#include "pepper-wayland.h"
#include <wayland-client.h>
#include <pixman.h>
#include <pepper-output-backend.h>
#include <pepper-input-backend.h>
#include <pepper-render.h>
#include <pepper-pixman-renderer.h>
#include <pepper-gl-renderer.h>

#if ENABLE_WAYLAND_BACKEND_EGL
#include <wayland-egl.h>
#endif

#define NUM_SHM_BUFFERS 2

typedef struct wayland_output       wayland_output_t;
typedef struct wayland_seat         wayland_seat_t;
typedef struct wayland_shm_buffer   wayland_shm_buffer_t;

struct pepper_wayland {
	pepper_compositor_t    *pepper;

	char                   *socket_name;
	struct wl_display      *display;
	int                     fd;

	struct wl_event_source *event_source;

	struct wl_registry     *registry;
	struct wl_compositor   *compositor;
	struct wl_shell        *shell;
	pepper_list_t           seat_list;
	pepper_list_t           output_list;

	struct wl_signal        destroy_signal;

	struct wl_shm          *shm;

	pepper_renderer_t      *pixman_renderer;
	pepper_renderer_t      *gl_renderer;
};

struct wayland_shm_buffer {
	wayland_output_t       *output;
	pepper_list_t           link;

	struct wl_buffer       *buffer;

	void                   *pixels;
	int                     stride;
	int                     size;
	int                     w, h;

	pepper_render_target_t *target;
	pixman_region32_t       damage;

	void                   *data;
};

struct wayland_output {
	pepper_wayland_t           *conn;
	pepper_output_t            *base;
	char                        name[32];

	struct wl_listener          conn_destroy_listener;

	int32_t                     w, h;
	int32_t                     subpixel;

	struct wl_surface          *surface;
	struct wl_shell_surface    *shell_surface;

	pepper_renderer_t          *renderer;
	pepper_render_target_t     *render_target;
	pepper_render_target_t     *gl_render_target;

	void    (*render_pre)(wayland_output_t *output);
	void    (*render_post)(wayland_output_t *output);

	struct {
		/* list containing free buffers. */
		pepper_list_t           free_buffers;

		/* list containing attached but not released (from the compositor) buffers. */
		pepper_list_t           attached_buffers;

		/* current render target buffer. */
		wayland_shm_buffer_t   *current_buffer;
	} shm;

#if ENABLE_WAYLAND_BACKEND_EGL
	struct {
		struct wl_egl_window   *window;
	} egl;
#endif

	pepper_plane_t             *primary_plane;
	pepper_list_t               link;
};

struct wayland_seat {
	pepper_wayland_t           *conn;
	struct wl_seat             *seat;
	uint32_t                    id;

	struct {
		pepper_input_device_t      *base;
		struct wl_pointer          *wl_pointer;
	} pointer;

	struct {
		pepper_input_device_t      *base;
		struct wl_keyboard         *wl_keyboard;
	} keyboard;

	struct {
		pepper_input_device_t      *base;
		struct wl_touch            *wl_touch;
	} touch;

	pepper_list_t                   link;
};

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version);

wayland_shm_buffer_t *
wayland_shm_buffer_create(wayland_output_t *output);

void
wayland_shm_buffer_destroy(wayland_shm_buffer_t *buffer);

char *
string_alloc(int len);

char *
string_copy(const char *str);

void
string_free(char *str);
