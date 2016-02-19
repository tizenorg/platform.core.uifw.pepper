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

typedef struct pepper_input_device_entry pepper_input_device_entry_t;

struct pepper_input_device_entry {
	pepper_seat_t              *seat;
	pepper_input_device_t      *device;
	pepper_event_listener_t    *listener;
	pepper_list_t               link;
};

static void
unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static const struct wl_seat_interface seat_interface = {
	pepper_pointer_bind_resource,
	pepper_keyboard_bind_resource,
	pepper_touch_bind_resource,
};

static void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	pepper_seat_t *seat = (pepper_seat_t *)data;
	struct wl_resource  *resource;

	resource = wl_resource_create(client, &wl_seat_interface, version/*FIXME*/, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&seat->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &seat_interface, data,
	                               unbind_resource);

	wl_seat_send_capabilities(resource, seat->caps);

	if ((seat->name) && (version >= WL_SEAT_NAME_SINCE_VERSION))
		wl_seat_send_name(resource, seat->name);
}

/**
 * Create and add a seat to the given compositor
 *
 * @param compositor    compositor object
 * @param seat_name     name of the seat to be added
 *
 * @return added seat
 *
 * Global for the wl_seat is created internally, thus broadcasted to all clients via registry.
 */
PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor,
                           const char *seat_name)
{
	pepper_seat_t  *seat;

	PEPPER_CHECK(seat_name, return NULL, "seat name must be given.\n");

	seat = (pepper_seat_t *)pepper_object_alloc(PEPPER_OBJECT_SEAT,
	                sizeof(pepper_seat_t));
	PEPPER_CHECK(seat, return NULL, "Failed to allocate memory in %s\n",
	             __FUNCTION__);

	pepper_list_init(&seat->link);
	wl_list_init(&seat->resource_list);
	pepper_list_init(&seat->input_device_list);

	seat->name = strdup(seat_name);
	PEPPER_CHECK(seat->name, goto error, "strdup() failed.\n");

	seat->global = wl_global_create(compositor->display, &wl_seat_interface, 4,
	                                seat, bind_seat);
	PEPPER_CHECK(seat->global, goto error, "wl_global_create() failed.\n");

	seat->compositor = compositor;

	seat->link.item = seat;
	pepper_list_insert(&compositor->seat_list, &seat->link);
	pepper_object_emit_event(&seat->compositor->base,
	                         PEPPER_EVENT_COMPOSITOR_SEAT_ADD, seat);

	return seat;

error:
	if (seat)
		pepper_seat_destroy(seat);

	return NULL;
}

/**
 * Destroy the given seat
 *
 * @param seat  seat object
 */
PEPPER_API void
pepper_seat_destroy(pepper_seat_t *seat)
{
	struct wl_resource *resource, *tmp;

	if (seat->name)
		free(seat->name);

	if (seat->pointer)
		pepper_pointer_destroy(seat->pointer);

	if (seat->keyboard)
		pepper_keyboard_destroy(seat->keyboard);

	if (seat->touch)
		pepper_touch_destroy(seat->touch);

	if (seat->global)
		wl_global_destroy(seat->global);

	wl_resource_for_each_safe(resource, tmp, &seat->resource_list)
	wl_resource_destroy(resource);
}

/**
 * Get the list of wl_resource of the given seat
 *
 * @param seat  seat object
 *
 * @return list of seat resource
 */
PEPPER_API struct wl_list *
pepper_seat_get_resource_list(pepper_seat_t *seat)
{
	return &seat->resource_list;
}

/**
 * Get the compositor of the given seat
 *
 * @param seat  seat object
 *
 * @return compositor of the seat
 */
PEPPER_API pepper_compositor_t *
pepper_seat_get_compositor(pepper_seat_t *seat)
{
	return seat->compositor;
}

/**
 * Get the pointer of the given seat
 *
 * @param seat  seat object
 *
 * @return pointer if exist, NULL otherwise
 *
 * When the seat doesn't have pointer capability, NULL would be returned.
 */
PEPPER_API pepper_pointer_t *
pepper_seat_get_pointer(pepper_seat_t *seat)
{
	return seat->pointer;
}

/**
 * Get the keyboard of the given seat
 *
 * @param seat  keyboard object
 *
 * @return keyboard to the keyboard object if exist, NULL otherwise
 *
 * When the seat doesn't have keyboard capability, NULL would be returned.
 */
PEPPER_API pepper_keyboard_t *
pepper_seat_get_keyboard(pepper_seat_t *seat)
{
	return seat->keyboard;
}

/**
 * Get the touch of the given seat
 *
 * @param seat  touch object
 *
 * @return touch to the touch object if exist, NULL otherwise
 *
 * When the seat doesn't have touch capability, NULL would be returned.
 */
PEPPER_API pepper_touch_t *
pepper_seat_get_touch(pepper_seat_t *seat)
{
	return seat->touch;
}

/**
 * Get the name of the given seat
 *
 * @param seat  seat object
 *
 * @return pointer the null terminating string of the seat name
 */
PEPPER_API const char *
pepper_seat_get_name(pepper_seat_t *seat)
{
	return seat->name;
}

static void
seat_update_pointer_cap(pepper_seat_t *seat)
{
	if ((seat->caps & WL_SEAT_CAPABILITY_POINTER) && !seat->pointer) {
		seat->pointer = pepper_pointer_create(seat);
		pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_POINTER_ADD,
		                         seat->pointer);
	} else if (!(seat->caps & WL_SEAT_CAPABILITY_POINTER) && seat->pointer) {
		pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_POINTER_REMOVE,
		                         seat->pointer);
		pepper_pointer_destroy(seat->pointer);
		seat->pointer = NULL;
	}
}

static void
seat_update_keyboard_cap(pepper_seat_t *seat)
{
	if ((seat->caps & WL_SEAT_CAPABILITY_KEYBOARD) && !seat->keyboard) {
		seat->keyboard = pepper_keyboard_create(seat);
		pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_KEYBOARD_ADD,
		                         seat->keyboard);
	} else if (!(seat->caps & WL_SEAT_CAPABILITY_KEYBOARD) && seat->keyboard) {
		pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_KEYBOARD_REMOVE,
		                         seat->keyboard);
		pepper_keyboard_destroy(seat->keyboard);
		seat->keyboard = NULL;
	}
}

static void
seat_update_touch_cap(pepper_seat_t *seat)
{
	if ((seat->caps & WL_SEAT_CAPABILITY_TOUCH) && !seat->touch) {
		seat->touch = pepper_touch_create(seat);
		pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_TOUCH_ADD, seat->touch);
	} else if (!(seat->caps & WL_SEAT_CAPABILITY_TOUCH) && seat->touch) {
		pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_TOUCH_REMOVE,
		                         seat->touch);
		pepper_touch_destroy(seat->touch);
		seat->touch = NULL;
	}
}

static void
seat_update_caps(pepper_seat_t *seat)
{
	uint32_t                        caps = 0;
	pepper_input_device_entry_t    *entry;

	pepper_list_for_each(entry, &seat->input_device_list, link)
	caps |= entry->device->caps;

	if (caps != seat->caps) {
		struct wl_resource *resource;

		seat->caps = caps;

		seat_update_pointer_cap(seat);
		seat_update_keyboard_cap(seat);
		seat_update_touch_cap(seat);

		wl_resource_for_each(resource, &seat->resource_list)
		wl_seat_send_capabilities(resource, seat->caps);
	}
}

static void
seat_handle_device_event(pepper_event_listener_t *listener,
                         pepper_object_t *object,
                         uint32_t id, void *info, void *data)
{
	pepper_input_device_entry_t *entry = data;
	pepper_seat_t               *seat = entry->seat;

	switch (id) {
	case PEPPER_EVENT_OBJECT_DESTROY:
		pepper_seat_remove_input_device(seat, entry->device);
		break;
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE:
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION:
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON:
	case PEPPER_EVENT_INPUT_DEVICE_POINTER_AXIS:
		pepper_pointer_handle_event(seat->pointer, id, info);
		break;
	case PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY:
		pepper_keyboard_handle_event(seat->keyboard, id, info);
		break;
	case PEPPER_EVENT_INPUT_DEVICE_TOUCH_DOWN:
	case PEPPER_EVENT_INPUT_DEVICE_TOUCH_UP:
	case PEPPER_EVENT_INPUT_DEVICE_TOUCH_MOTION:
	case PEPPER_EVENT_INPUT_DEVICE_TOUCH_FRAME:
		pepper_touch_handle_event(seat->touch, id, info);
		break;
	}
}

/**
 * Add the input device to the given seat
 *
 * @param seat      seat object
 * @param device    input device object
 *
 * Seat's capabilities will be updated according to the device's capabilities and the change is
 * broadcasted to all resources. The seat installs an event listener on the device and dispatches
 * input events to proper destination which is one of pepper_pointer/keyboard/touch_t of the seat.
 * Device add events are emitted to the seat according to the device's capabilities like
 * PEPPER_EVENT_SEAT_POINTER_DEVICE_ADD. If any of pepper_pointer/keyboard/touch_t has been created
 * by adding the input device, event is emitted to the seat like PEPPER_EVENT_SEAT_POINTER_ADD.
 */
PEPPER_API void
pepper_seat_add_input_device(pepper_seat_t *seat, pepper_input_device_t *device)
{
	pepper_input_device_entry_t *entry;

	pepper_list_for_each(entry, &seat->input_device_list, link) {
		if (entry->device == device)
			return;
	}

	entry = calloc(1, sizeof(pepper_input_device_entry_t));
	PEPPER_CHECK(entry, return, "calloc() failed.\n");

	entry->seat = seat;
	entry->device = device;
	pepper_list_insert(&seat->input_device_list, &entry->link);

	seat_update_caps(seat);

	entry->listener = pepper_object_add_event_listener(&device->base,
	                  PEPPER_EVENT_ALL, 0,
	                  seat_handle_device_event, entry);
}

/**
 * Remove an input device from the given seat
 *
 * @param seat      seat to remove the input device
 * @param device    input device to be removed
 *
 * If the device is not added to the seat, this function has no effect. Removing the device causes
 * the capabilities of the seat to change resulting wayland events and pepper events. Refer to the
 * events described in pepper_seat_add_input_device().
 */
PEPPER_API void
pepper_seat_remove_input_device(pepper_seat_t *seat,
                                pepper_input_device_t *device)
{
	pepper_input_device_entry_t *entry;

	pepper_list_for_each(entry, &seat->input_device_list, link) {
		if (entry->device == device) {
			pepper_list_remove(&entry->link);
			pepper_event_listener_remove(entry->listener);
			free(entry);
			seat_update_caps(seat);
			return;
		}
	}
}

/**
 * Create an input device.
 *
 * @param compositor    compositor to add the device
 * @param caps          capabilities of the device
 * @param backend       pointer to an input device backend function table
 * @param data          backend data
 *
 * @returns             #pepper_input_device_t
 *
 * PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD event is emitted.
 */
PEPPER_API pepper_input_device_t *
pepper_input_device_create(pepper_compositor_t *compositor, uint32_t caps,
                           const pepper_input_device_backend_t *backend, void *data)
{
	pepper_input_device_t  *device;

	device = (pepper_input_device_t *)pepper_object_alloc(
	                 PEPPER_OBJECT_INPUT_DEVICE,
	                 sizeof(pepper_input_device_t));
	PEPPER_CHECK(device, return NULL, "pepper_object_alloc() failed.\n");

	device->compositor = compositor;
	device->caps = caps;
	device->backend = backend;
	device->data = data;
	device->link.item = device;

	pepper_list_insert(&compositor->input_device_list, &device->link);
	pepper_object_emit_event(&compositor->base,
	                         PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
	                         device);
	return device;
}

/**
 * Destroy the given input device.
 *
 * @param device    input device to destroy
 *
 * PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE event is emitted.
 */
PEPPER_API void
pepper_input_device_destroy(pepper_input_device_t *device)
{
	pepper_object_emit_event(&device->compositor->base,
	                         PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE, device);
	pepper_list_remove(&device->link);
	pepper_object_fini(&device->base);
	free(device);
}

/**
 * Get the #pepper_compositor_t of the given #pepper_input_device_t
 *
 * @param device    input device to get the compositor
 *
 * @return compositor
 */
PEPPER_API pepper_compositor_t *
pepper_input_device_get_compositor(pepper_input_device_t *device)
{
	return device->compositor;
}

/**
 * Get property of the given #pepper_input_device_t for the given key
 *
 * @param device    input device to get the property
 * @param key       key for the property
 *
 * @return null terminating string of the property if exist, NULL otherwise
 *
 * Available keys for the properties are different between input backends.
 */
PEPPER_API const char *
pepper_input_device_get_property(pepper_input_device_t *device, const char *key)
{
	if (!device->backend)
		return NULL;

	return device->backend->get_property(device->data, key);
}

/**
 * Get capabilities value of the given input device.
 *
 * @param device    device to get capabilities
 *
 * @returns         capabilities of the device
 *
 * @see wl_seat_capability
 */
PEPPER_API uint32_t
pepper_input_device_get_caps(pepper_input_device_t *device)
{
	return device->caps;
}
