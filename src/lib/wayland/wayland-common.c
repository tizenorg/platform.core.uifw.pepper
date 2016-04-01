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
#include <string.h>
#include <stdlib.h>

char *
string_alloc(int len)
{
	return (char *)malloc((len + 1) * sizeof (char));
}

char *
string_copy(const char *str)
{
	int len = strlen(str);
	char *ret = string_alloc(len);

	if (ret)
		memcpy(ret, str, (len + 1) * sizeof (char));

	return ret;
}

void
string_free(char *str)
{
	free(str);
}

static void
handle_global(void *data, struct wl_registry *registry,
			  uint32_t name, const char *interface, uint32_t version)
{
	pepper_wayland_t *conn = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		conn->compositor = wl_registry_bind(registry, name, &wl_compositor_interface,
											1);
	} else if (strcmp(interface, "wl_seat") == 0) {
		wayland_handle_global_seat(conn, registry, name, version);
	} else if (strcmp(interface, "wl_shell") == 0) {
		conn->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
	} else if (strcmp(interface, "wl_shm") == 0) {
		conn->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	/* TODO: */
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	handle_global_remove,
};

static int
handle_wayland_event(int fd, uint32_t mask, void *data)
{
	pepper_wayland_t *conn = data;
	int count = 0;

	if ((mask & WL_EVENT_HANGUP) || (mask & WL_EVENT_ERROR))
		return 0;

	if (mask & WL_EVENT_READABLE)
		count = wl_display_dispatch(conn->display);

	if (mask & WL_EVENT_WRITABLE)
		wl_display_flush(conn->display);

	if (mask == 0) {
		count = wl_display_dispatch_pending(conn->display);
		wl_display_flush(conn->display);
	}

	return count;
}

PEPPER_API pepper_wayland_t *
pepper_wayland_connect(pepper_compositor_t *compositor, const char *socket_name)
{
	pepper_wayland_t        *conn;
	struct wl_display       *compositor_display;
	struct wl_event_loop    *loop;

	conn = (pepper_wayland_t *)calloc(1, sizeof(pepper_wayland_t));
	if (!conn)
		return NULL;

	conn->pepper = compositor;

	conn->socket_name = string_copy(socket_name);
	conn->display = wl_display_connect(socket_name);
	conn->fd = wl_display_get_fd(conn->display);

	conn->gl_renderer = pepper_gl_renderer_create(compositor, conn->display,
						"wayland");
	conn->pixman_renderer = pepper_pixman_renderer_create(compositor);

	if (!conn->pixman_renderer) {
		free(conn);
		return NULL;
	}

	compositor_display = pepper_compositor_get_display(compositor);
	loop = wl_display_get_event_loop(compositor_display);
	conn->event_source = wl_event_loop_add_fd(loop, conn->fd, WL_EVENT_READABLE,
						 handle_wayland_event, conn);
	wl_event_source_check(conn->event_source);

	pepper_list_init(&conn->seat_list);
	pepper_list_init(&conn->output_list);

	conn->registry = wl_display_get_registry(conn->display);
	wl_registry_add_listener(conn->registry, &registry_listener, conn);
	wl_display_roundtrip(conn->display);

	return conn;
}

PEPPER_API void
pepper_wayland_destroy(pepper_wayland_t *conn)
{
	wayland_output_t *output, *tmp;

	pepper_list_for_each_safe(output, tmp, &conn->output_list, link)
	pepper_output_destroy(output->base);

	if (conn->pixman_renderer)
		pepper_renderer_destroy(conn->pixman_renderer);

	if (conn->gl_renderer)
		pepper_renderer_destroy(conn->gl_renderer);

	if (conn->socket_name)
		string_free(conn->socket_name);

	if (conn->event_source)
		wl_event_source_remove(conn->event_source);

	if (conn->registry)
		wl_registry_destroy(conn->registry);

	if (conn->compositor)
		wl_compositor_destroy(conn->compositor);

	if (conn->shell)
		wl_shell_destroy(conn->shell);

	free(conn);
}
