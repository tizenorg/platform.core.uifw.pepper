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

#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "pepper-internal.h"

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_impl = {
	keyboard_release,
};

static void
clear_keymap(pepper_keyboard_t *keyboard)
{
	if (keyboard->state) {
		xkb_state_unref(keyboard->state);
		keyboard->state = NULL;
	}

	if (keyboard->keymap) {
		xkb_keymap_unref(keyboard->keymap);
		keyboard->keymap = NULL;
	}

	if (keyboard->keymap_fd >= 0) {
		close(keyboard->keymap_fd);
		keyboard->keymap_fd = -1;
		keyboard->keymap_len = -1;
	}

	if (keyboard->pending_keymap) {
		xkb_keymap_unref(keyboard->pending_keymap);
		keyboard->pending_keymap = NULL;
	}
}

static void
update_modifiers(pepper_keyboard_t *keyboard)
{
	uint32_t mods_depressed, mods_latched, mods_locked, group;

	mods_depressed = xkb_state_serialize_mods(keyboard->state,
					 XKB_STATE_MODS_DEPRESSED);
	mods_latched = xkb_state_serialize_mods(keyboard->state,
											XKB_STATE_MODS_LATCHED);
	mods_locked = xkb_state_serialize_mods(keyboard->state, XKB_STATE_MODS_LOCKED);
	group = xkb_state_serialize_mods(keyboard->state, XKB_STATE_LAYOUT_EFFECTIVE);

	if ((mods_depressed != keyboard->mods_depressed) ||
		(mods_latched != keyboard->mods_latched) ||
		(mods_locked != keyboard->mods_locked) || (group != keyboard->group)) {
		keyboard->mods_depressed = mods_depressed;
		keyboard->mods_latched = mods_latched;
		keyboard->mods_locked = mods_locked;
		keyboard->group = group;

		keyboard->grab->modifiers(keyboard, keyboard->data, mods_depressed,
								  mods_latched,
								  mods_locked, group);
	}
}

static void
update_key(pepper_keyboard_t *keyboard, uint32_t key, uint32_t state)
{
	enum xkb_key_direction  direction;

	if (!keyboard->state)
		return;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		direction = XKB_KEY_DOWN;
	else
		direction = XKB_KEY_UP;

	xkb_state_update_key(keyboard->state, key + 8, direction);
	update_modifiers(keyboard);
}

static void
update_keymap(pepper_keyboard_t *keyboard)
{
	struct wl_resource             *resource;
	char                           *keymap_str = NULL;
	char                           *keymap_map = NULL;

	struct xkb_state               *state;
	uint32_t                        mods_latched = 0;
	uint32_t                        mods_locked = 0;
	uint32_t                        format;

	if (keyboard->keymap)
		xkb_keymap_unref(keyboard->keymap);

	if (keyboard->keymap_fd)
		close(keyboard->keymap_fd);

	if (keyboard->pending_keymap) {
		format = WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1;
		keyboard->keymap = xkb_keymap_ref(keyboard->pending_keymap);
		xkb_keymap_unref(keyboard->pending_keymap);
		keyboard->pending_keymap = NULL;

		keymap_str = xkb_keymap_get_as_string(keyboard->keymap,
											  XKB_KEYMAP_FORMAT_TEXT_V1);
		PEPPER_CHECK(keymap_str, goto error, "failed to get keymap string\n");

		keyboard->keymap_len = strlen(keymap_str) + 1;
		keyboard->keymap_fd = pepper_create_anonymous_file(keyboard->keymap_len);
		PEPPER_CHECK(keyboard->keymap_fd, goto error, "failed to create keymap file\n");

		keymap_map = mmap(NULL, keyboard->keymap_len, PROT_READ | PROT_WRITE,
						  MAP_SHARED,
						  keyboard->keymap_fd, 0);
		PEPPER_CHECK(keymap_map, goto error, "failed to mmap for keymap\n");

		strcpy(keymap_map, keymap_str);

		state = xkb_state_new(keyboard->keymap);
		PEPPER_CHECK(state, goto error, "failed to create xkb state\n");

		if (keyboard->state) {
			mods_latched = xkb_state_serialize_mods(keyboard->state,
													XKB_STATE_MODS_LATCHED);
			mods_locked = xkb_state_serialize_mods(keyboard->state, XKB_STATE_MODS_LOCKED);
			xkb_state_update_mask(state, 0, mods_latched, mods_locked, 0, 0, 0);
			xkb_state_unref(keyboard->state);
		}

		keyboard->state = state;
	} else {
		format = WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP;
	}

	wl_resource_for_each(resource, &keyboard->resource_list)
	wl_keyboard_send_keymap(resource, format, keyboard->keymap_fd,
							keyboard->keymap_len);

	update_modifiers(keyboard);

	if (!mods_latched && !mods_locked)
		goto done;

	wl_resource_for_each(resource, &keyboard->resource_list)
	wl_keyboard_send_modifiers(resource,
							   wl_display_next_serial(keyboard->seat->compositor->display),
							   keyboard->mods_depressed, keyboard->mods_latched,
							   keyboard->mods_locked, keyboard->group);
	goto done;

error:
	clear_keymap(keyboard);

done:
	if (keymap_map)
		munmap(keymap_map, keyboard->keymap_len);

	if (keymap_str)
		free(keymap_str);
}

void
pepper_keyboard_handle_event(pepper_keyboard_t *keyboard, uint32_t id,
							 pepper_input_event_t *event)
{
	uint32_t       *keys = keyboard->keys.data;
	unsigned int    num_keys = keyboard->keys.size / sizeof(uint32_t);
	unsigned int    i;

	if (id != PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY)
		return;

	/* Update the array of pressed keys. */
	for (i = 0; i < num_keys; i++) {
		if (keys[i] == event->key) {
			keys[i] = keys[--num_keys];
			break;
		}
	}

	keyboard->keys.size = num_keys * sizeof(uint32_t);

	if (event->state == PEPPER_KEY_STATE_PRESSED)
		*(uint32_t *)wl_array_add(&keyboard->keys, sizeof(uint32_t)) = event->key;

	if (keyboard->grab)
		keyboard->grab->key(keyboard, keyboard->data, event->time, event->key,
							event->state);

	if (keyboard->pending_keymap && (keyboard->keys.size == 0))
		update_keymap(keyboard);

	update_key(keyboard, event->key, event->state);

	pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_KEYBOARD_KEY, event);
}

static void
keyboard_handle_focus_destroy(pepper_event_listener_t *listener,
							  pepper_object_t *surface,
							  uint32_t id, void *info, void *data)
{
	pepper_keyboard_t *keyboard = data;
	pepper_keyboard_set_focus(keyboard, NULL);

	if (keyboard->grab)
		keyboard->grab->cancel(keyboard, keyboard->data);
}

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat)
{
	pepper_keyboard_t *keyboard =
		(pepper_keyboard_t *)pepper_object_alloc(PEPPER_OBJECT_KEYBOARD,
				sizeof(pepper_keyboard_t));

	PEPPER_CHECK(keyboard, return NULL, "pepper_object_alloc() failed.\n");

	keyboard->seat = seat;
	wl_list_init(&keyboard->resource_list);

	wl_array_init(&keyboard->keys);

	return keyboard;
}

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard)
{
	clear_keymap(keyboard);

	if (keyboard->grab)
		keyboard->grab->cancel(keyboard, keyboard->data);

	if (keyboard->focus)
		pepper_event_listener_remove(keyboard->focus_destroy_listener);

	wl_array_release(&keyboard->keys);
	free(keyboard);
}

static void
unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

void
pepper_keyboard_bind_resource(struct wl_client *client,
							  struct wl_resource *resource, uint32_t id)
{
	pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
	pepper_keyboard_t  *keyboard = seat->keyboard;
	struct wl_resource *res;

	if (!keyboard)
		return;

	res = wl_resource_create(client, &wl_keyboard_interface,
							 wl_resource_get_version(resource), id);
	if (!res) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&keyboard->resource_list, wl_resource_get_link(res));
	wl_resource_set_implementation(res, &keyboard_impl, keyboard, unbind_resource);

	/* TODO: send repeat info */

	if (keyboard->keymap) {
		wl_keyboard_send_keymap(res, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
								keyboard->keymap_fd, keyboard->keymap_len);
	} else {
		int fd = open("/dev/null", O_RDONLY);
		wl_keyboard_send_keymap(res, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP, fd, 0);
		close(fd);
	}

	if (!keyboard->focus || !keyboard->focus->surface ||
		!keyboard->focus->surface->resource)
		return;

	if (wl_resource_get_client(keyboard->focus->surface->resource) == client) {
		wl_keyboard_send_enter(res, keyboard->focus_serial,
							   keyboard->focus->surface->resource, &keyboard->keys);
	}
}

/**
 * Get the list of wl_resource of the given keyboard
 *
 * @param keyboard  keyboard object
 *
 * @return list of the keyboard resources
 */
PEPPER_API struct wl_list *
pepper_keyboard_get_resource_list(pepper_keyboard_t *keyboard)
{
	return &keyboard->resource_list;
}

/**
 * Get the compositor of the given keyboard
 *
 * @param keyboard  keyboard object
 *
 * @return compositor
 */
PEPPER_API pepper_compositor_t *
pepper_keyboard_get_compositor(pepper_keyboard_t *keyboard)
{
	return keyboard->seat->compositor;
}

/**
 * Get the seat of the given keyboard
 *
 * @param keyboard  keyboard object
 *
 * @return seat
 */
PEPPER_API pepper_seat_t *
pepper_keyboard_get_seat(pepper_keyboard_t *keyboard)
{
	return keyboard->seat;
}

/**
 * Set the focus view of the given keyboard
 *
 * @param keyboard  keyboard object
 * @param focus     focus view
 *
 * @see pepper_keyboard_send_enter()
 * @see pepper_keyboard_send_leave()
 * @see pepper_keyboard_get_focus()
 */
PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *focus)
{
	if (keyboard->focus == focus)
		return;

	if (keyboard->focus) {
		pepper_event_listener_remove(keyboard->focus_destroy_listener);
		pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_FOCUS_LEAVE,
								 keyboard->focus);
		pepper_object_emit_event(&keyboard->focus->base, PEPPER_EVENT_FOCUS_LEAVE,
								 keyboard);
	}

	keyboard->focus = focus;

	if (focus) {
		keyboard->focus_serial = wl_display_next_serial(
									 keyboard->seat->compositor->display);

		keyboard->focus_destroy_listener =
			pepper_object_add_event_listener(&focus->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
											 keyboard_handle_focus_destroy, keyboard);

		pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_FOCUS_ENTER, focus);
		pepper_object_emit_event(&focus->base, PEPPER_EVENT_FOCUS_ENTER, keyboard);
	}
}

/**
 * Get the focus view of the given keyboard
 *
 * @param keyboard   keyboard object
 *
 * @return focus view
 *
 * @see pepper_keyboard_set_focus()
 */
PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard)
{
	return keyboard->focus;
}

/**
 * Send wl_keyboard.leave event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the leave event
 */
PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard, pepper_view_t *view)
{
	struct wl_resource *resource;
	struct wl_client   *client;
	uint32_t            serial;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_leave(resource, serial, view->surface->resource);
	}
}

/**
 * Send wl_keyboard.enter event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the enter event
 */
PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard, pepper_view_t *view)
{
	struct wl_resource *resource;
	struct wl_client   *client;
	uint32_t            serial;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_enter(resource, serial, view->surface->resource,
								   &keyboard->keys);
	}
}

/**
 * Send wl_keyboard.key event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the enter event
 * @param time      time in mili-second with undefined base
 * @param key       key code
 * @param state     state flag (ex. WL_KEYBOARD_KEY_STATE_PRESSED)
 */
PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, pepper_view_t *view,
						 uint32_t time, uint32_t key, uint32_t state)
{
	struct wl_resource     *resource;
	struct wl_client       *client;
	uint32_t                serial;
	pepper_input_event_t    event;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_key(resource, serial, time, key, state);
	}

	event.time = time;
	event.key = key;
	event.state = state;
	pepper_object_emit_event(&view->base, PEPPER_EVENT_KEYBOARD_KEY, &event);
}

/**
 * Send wl_keyboard.key event to the client
 *
 * @param keyboard  keyboard object
 * @param view      view object having the target surface for the enter event
 * @param depressed (none)
 * @param latched   (none)
 * @param locked    (none)
 * @param group     (none)
 */
PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard, pepper_view_t *view,
							   uint32_t depressed, uint32_t latched,
							   uint32_t locked, uint32_t group)
{
	struct wl_resource *resource;
	struct wl_client   *client;
	uint32_t            serial;

	if (!view || !view->surface || !view->surface->resource)
		return;

	client = wl_resource_get_client(view->surface->resource);
	serial = wl_display_next_serial(keyboard->seat->compositor->display);

	wl_resource_for_each(resource, &keyboard->resource_list) {
		if (wl_resource_get_client(resource) == client)
			wl_keyboard_send_modifiers(resource, serial, depressed, latched, locked, group);
	}
}

/**
 * Install keyboard grab
 *
 * @param keyboard  keyboard object
 * @param grab      grab handler
 * @param data      user data to be passed to grab functions
 */
PEPPER_API void
pepper_keyboard_set_grab(pepper_keyboard_t *keyboard,
						 const pepper_keyboard_grab_t *grab, void *data)
{
	keyboard->grab = grab;
	keyboard->data = data;
}

/**
 * Get the current keyboard grab
 *
 * @param keyboard  keyboard object
 *
 * @return grab handler which is most recently installed
 *
 * @see pepper_keyboard_set_grab()
 * @see pepper_keyboard_get_grab_data()
 */
PEPPER_API const pepper_keyboard_grab_t *
pepper_keyboard_get_grab(pepper_keyboard_t *keyboard)
{
	return keyboard->grab;
}

/**
 * Get the current keyboard grab data
 *
 * @param keyboard  keyboard object
 *
 * @return grab data which is most recently installed
 *
 * @see pepper_keyboard_set_grab()
 * @see pepper_keyboard_get_grab()
 */
PEPPER_API void *
pepper_keyboard_get_grab_data(pepper_keyboard_t *keyboard)
{
	return keyboard->data;
}

/**
 * Set xkb keymap for the given keyboard
 *
 * @param keyboard  keyboard object
 * @param keymap    xkb keymap
 *
 * This function might send wl_pointer.keymap and wl_pointer.modifers events internally
 */
PEPPER_API void
pepper_keyboard_set_keymap(pepper_keyboard_t *keyboard,
						   struct xkb_keymap *keymap)
{
	xkb_keymap_unref(keyboard->pending_keymap);
	if (keymap)
		keyboard->pending_keymap = xkb_keymap_ref(keymap);
	else
		keyboard->pending_keymap = NULL;

	if (keyboard->keys.size == 0)
		update_keymap(keyboard);
}
