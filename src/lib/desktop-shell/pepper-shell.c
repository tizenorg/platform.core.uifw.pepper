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

#include "desktop-shell-internal.h"
#include "pepper-shell-server-protocol.h"

static void
pepper_shell_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
pepper_shell_set_cursor(struct wl_client *client, struct wl_resource *resource,
                        struct wl_resource *surface_resource)
{
	desktop_shell_t  *shell = wl_resource_get_user_data(resource);
	pepper_surface_t *surface = wl_resource_get_user_data(surface_resource);

	if (surface != shell->cursor_surface) {
		if (!shell->cursor_view) {
			shell->cursor_view = pepper_compositor_add_view(shell->compositor);

			if (!shell->cursor_view) {
				wl_client_post_no_memory(client);
				return;
			}
		}

		if (!pepper_surface_set_role(surface, "pepper_cursor")) {
			wl_resource_post_error(resource, PEPPER_SHELL_ERROR_ROLE, "Already has a role");
			return;
		}

		pepper_view_set_surface(shell->cursor_view, surface);
		pepper_view_map(shell->cursor_view);

		/* TODO: Cursor view management. */
	}
}

static const struct pepper_shell_interface pepper_shell_implementation = {
	pepper_shell_destroy,
	pepper_shell_set_cursor,
};

static void
unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
bind_pepper_shell(struct wl_client *client, void *data, uint32_t version,
                  uint32_t id)
{
	desktop_shell_t     *shell = data;
	struct wl_resource  *resource;

	resource = wl_resource_create(client, &pepper_shell_interface, version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&shell->pepper_shell_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &pepper_shell_implementation, shell,
	                               unbind_resource);
	wl_resource_set_user_data(resource, shell);
}

pepper_bool_t
init_pepper_shell(desktop_shell_t *shell)
{
	struct wl_display  *display = pepper_compositor_get_display(shell->compositor);

	shell->pepper_shell_global =
	        wl_global_create(display, &pepper_shell_interface, 1, shell, bind_pepper_shell);

	if (!shell->pepper_shell_global)
		return PEPPER_FALSE;

	return PEPPER_TRUE;
}

void
fini_pepper_shell(desktop_shell_t *shell)
{
	struct wl_resource *res, *tmp;

	if (shell->pepper_shell_global) {
		wl_global_destroy(shell->pepper_shell_global);
		shell->pepper_shell_global = NULL;
	}

	wl_resource_for_each_safe(res, tmp, &shell->pepper_shell_list)
	wl_resource_destroy(res);
}
