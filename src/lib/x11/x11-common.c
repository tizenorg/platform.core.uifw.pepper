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

#include <wayland-server.h>
#include <pepper-pixman-renderer.h>
#include <pepper-gl-renderer.h>
#include "x11-internal.h"

#include <stdlib.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

/*
 * xcb version
 * xcb is faster than xlib
 */

static inline pepper_bool_t
x11_get_next_event(xcb_connection_t *xcb_conn, xcb_generic_event_t **event,
                   uint32_t mask)
{
	*event = xcb_poll_for_event(xcb_conn);
	return *event != NULL;
}

static x11_output_t *
x11_find_output_by_window(pepper_x11_connection_t *conn, xcb_window_t window)
{
	x11_output_t *output;

	pepper_list_for_each(output, &conn->output_list, link) {
		if ( window == output->window )
			return output;
	}

	return NULL;
}

static int
x11_handle_event(int fd, uint32_t mask, void *data)
{
	pepper_x11_connection_t     *connection = data;
	x11_seat_t                  *seat;
	xcb_generic_event_t         *event = NULL;

	uint32_t                    count = 0;

	/* TODO: At now, x11-backend has only 1 seat per connection, "seat0"
	 *       but if not, we need to find matched seat at here
	 */
	seat = connection->seat;

	while (x11_get_next_event(connection->xcb_connection, &event, mask)) {
		uint32_t type = event->response_type & ~0x80;

		switch (type) {
		case XCB_ENTER_NOTIFY:
		case XCB_LEAVE_NOTIFY:
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
		case XCB_MOTION_NOTIFY:
			if (connection->use_xinput)
				x11_handle_input_event(seat, type, event);
			break;
		case XCB_EXPOSE:
			PEPPER_TRACE("XCB_EXPOSE\n");
			{
				xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
				x11_output_t *output = x11_find_output_by_window(connection, expose->window);

				if (output) {
					/* TODO: Damage only newly exposed area. */
					pepper_output_add_damage_region(output->base, NULL);
				}
			}
			break;
		case XCB_FOCUS_IN:
			/* TODO: Cursor handling */
			PEPPER_TRACE("XCB_FOCUS_IN\n");
			break;
		case XCB_FOCUS_OUT:
			PEPPER_TRACE("XCB_FOCUS_OUT\n");
			break;
		case XCB_KEYMAP_NOTIFY:
			PEPPER_TRACE("XCB_KEYMAP_NOTIFY\n");
			{
				/* TODO: ignore at this time. */
			}
			break;
		case XCB_CONFIGURE_NOTIFY:
			/* Window moved */
			PEPPER_TRACE("XCB_CONFIGURE_NOTIFY\n");
			break;
		case XCB_QUERY_EXTENSION:
			PEPPER_TRACE("XCB_QUERY_EXTENSION\n");
			break;
		case XCB_DESTROY_NOTIFY:
			PEPPER_TRACE("XCB_DESTROY_NOTIFY\n");
			break;
		case XCB_UNMAP_NOTIFY:
			PEPPER_TRACE("XCB_UNMAP_NOTIFY\n");
			break;
		case XCB_CLIENT_MESSAGE:
			PEPPER_TRACE("XCB_CLIENT_MESSAGE\n");
			{
				xcb_client_message_event_t *msg;
				xcb_atom_t                  atom;
				xcb_window_t                window;
				x11_output_t               *output = NULL;

				msg = (xcb_client_message_event_t *)event;
				atom = msg->data.data32[0];
				window = msg->window;

				if (atom == connection->atom.wm_delete_window) {
					output = x11_find_output_by_window(connection, window);
					if (output)
						x11_output_destroy(output);
				}
			}
			break;
		default :
			PEPPER_ERROR("unknown event: type [0x%x] \n", type);
			break;
		}

		free(event);
		count++;
	}

	return count;
}

#define F(field) offsetof(pepper_x11_connection_t, field)
static void
x11_init_atoms(pepper_x11_connection_t *conn)
{
	static const struct {
		const char *name;
		int        offset;
	} atoms[] = {
		{ "WM_PROTOCOLS",       F(atom.wm_protocols) },
		{ "WM_NORMAL_HINTS",    F(atom.wm_normal_hints) },
		{ "WM_SIZE_HINTS",      F(atom.wm_size_hints) },
		{ "WM_DELETE_WINDOW",   F(atom.wm_delete_window) },
		{ "WM_CLASS",           F(atom.wm_class) },
		{ "_NET_WM_NAME",       F(atom.net_wm_name) },
		{ "_NET_WM_ICON",       F(atom.net_wm_icon) },
		{ "_NET_WM_STATE",      F(atom.net_wm_state) },
		{ "_NET_WM_STATE_FULLSCREEN", F(atom.net_wm_state_fullscreen) },
		{ "_NET_SUPPORTING_WM_CHECK", F(atom.net_supporting_wm_check) },
		{ "_NET_SUPPORTED",     F(atom.net_supported) },
		{ "STRING",             F(atom.string) },
		{ "UTF8_STRING",        F(atom.utf8_string) },
		{ "CARDINAL",           F(atom.cardinal) },
	};

	xcb_intern_atom_cookie_t cookies[ARRAY_LENGTH(atoms)];
	xcb_intern_atom_reply_t  *reply;

	unsigned int i;

	for (i = 0; i < ARRAY_LENGTH(atoms); i++) {
		cookies[i] = xcb_intern_atom(conn->xcb_connection, 0,
		                             strlen(atoms[i].name),
		                             atoms[i].name);
	}

	for (i = 0; i < ARRAY_LENGTH(atoms); i++) {
		reply = xcb_intern_atom_reply(conn->xcb_connection, cookies[i], NULL);
		*(xcb_atom_t *) ((char *)conn + atoms[i].offset) = reply->atom;
		free(reply);
	}
}

PEPPER_API pepper_x11_connection_t *
pepper_x11_connect(pepper_compositor_t *compositor, const char *display_name)
{
	pepper_x11_connection_t     *connection = NULL;
	struct wl_display           *wdisplay;
	struct wl_event_loop        *loop;
	xcb_screen_iterator_t       scr_iter;

	if (!compositor) {
		PEPPER_ERROR("Compositor is null\n");
		return NULL;
	}

	connection = (pepper_x11_connection_t *)calloc(1,
	                sizeof(pepper_x11_connection_t));
	if (!connection) {
		PEPPER_ERROR("Memory allocation failed\n");
		return NULL;
	}

	connection->display = XOpenDisplay(display_name);
	if (!connection->display) {
		PEPPER_ERROR("XOpenDisplay failed\n");
		free(connection);
		return NULL;
	}

	connection->xcb_connection = XGetXCBConnection(connection->display);
	XSetEventQueueOwner(connection->display, XCBOwnsEventQueue);

	if (xcb_connection_has_error(connection->xcb_connection)) {
		PEPPER_ERROR("xcb connection has error\n");
		free(connection);
		return NULL;
	}
	connection->compositor = compositor;

	connection->gl_renderer = pepper_gl_renderer_create(connection->compositor,
	                          connection->display, "x11");

	connection->pixman_renderer = pepper_pixman_renderer_create(
	                                      connection->compositor);
	if (!connection->pixman_renderer) {
		PEPPER_ERROR("Failed to create pixman renderer.\n");
		free(connection);
		return NULL;
	}

	scr_iter = xcb_setup_roots_iterator(xcb_get_setup(connection->xcb_connection));
	connection->screen = scr_iter.data;

	connection->fd = xcb_get_file_descriptor(connection->xcb_connection);
	if (display_name)
		connection->display_name = strdup(display_name);
	else
		connection->display_name = NULL;

	x11_init_atoms(connection);

	wdisplay = pepper_compositor_get_display(compositor);
	loop = wl_display_get_event_loop(wdisplay);

	connection->xcb_event_source = wl_event_loop_add_fd(loop,
	                               connection->fd,
	                               WL_EVENT_READABLE,
	                               x11_handle_event,
	                               connection);

	wl_event_source_check(connection->xcb_event_source);
	pepper_list_init(&connection->output_list);

	return connection;
}

PEPPER_API void
pepper_x11_destroy(pepper_x11_connection_t *conn)
{
	x11_output_t *output, *tmp;

	x11_seat_destroy(conn->seat);

	pepper_list_for_each_safe(output, tmp, &conn->output_list, link)
	x11_output_destroy(output);

	if (conn->xcb_event_source)
		wl_event_source_remove(conn->xcb_event_source);

	if (conn->pixman_renderer)
		pepper_renderer_destroy(conn->pixman_renderer);

	if (conn->gl_renderer)
		pepper_renderer_destroy(conn->gl_renderer);

	XCloseDisplay(conn->display);
	free(conn);
}
