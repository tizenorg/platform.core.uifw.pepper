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
#include <stdlib.h>

void
shell_get_output_workarea(desktop_shell_t       *shell,
                          pepper_output_t       *output,
                          pixman_rectangle32_t  *area)
{
    const pepper_output_geometry_t *geom;

    /**
     ** TODO: Get given output's workarea size and position in global coordinate
     **      return (output_size - (panel_size + margin + caption + ... ));
     **/

    geom = pepper_output_get_geometry(output);

    if (area)
    {
        area->x = geom->x;
        area->y = geom->y;
        area->width = geom->w;
        area->height = geom->h;
    }
}

static void
handle_shell_client_destroy(struct wl_listener *listener, void *data)
{
    shell_client_t *shell_client = pepper_container_of(listener, shell_client,
                                                       client_destroy_listener);

    remove_ping_timer(shell_client);
    pepper_list_remove(&shell_client->link);
    free(shell_client);
}

shell_client_t *
shell_client_create(desktop_shell_t *shell, struct wl_client *client,
                    const struct wl_interface *interface, const void *implementation,
                    uint32_t version, uint32_t id)
{
    shell_client_t  *shell_client;

    shell_client = calloc(1, sizeof(shell_client_t));
    if (!shell_client)
    {
        wl_client_post_no_memory(client);
        return NULL;
    }

    shell_client->resource = wl_resource_create(client, interface, version, id);
    if (!shell_client->resource)
    {
        wl_client_post_no_memory(client);
        free(shell_client);
        return NULL;
    }

    shell_client->shell  = shell;
    shell_client->client = client;

    shell_client->client_destroy_listener.notify = handle_shell_client_destroy;
    wl_client_add_destroy_listener(client, &shell_client->client_destroy_listener);

    pepper_list_insert(&shell->shell_client_list, &shell_client->link);
    wl_resource_set_implementation(shell_client->resource, implementation, shell_client, NULL);

    return shell_client;
}

static void
shell_add_input_device(desktop_shell_t *shell, pepper_input_device_t *device)
{
    shell_seat_t            *shseat;
    pepper_seat_t           *seat;
    const char              *target_seat_name;
    const char              *seat_name;

    target_seat_name = pepper_input_device_get_property(device, "seat_name");
    if (!target_seat_name)
        target_seat_name = "seat0";

    pepper_list_for_each(shseat, &shell->shseat_list, link)
    {
        seat_name = pepper_seat_get_name(shseat->seat);

        /* Find seat to adding input device */
        if ( seat_name && !strcmp(seat_name, target_seat_name))
        {
            pepper_seat_add_input_device(shseat->seat, device);
            return ;
        }
    }

    seat = pepper_compositor_add_seat(shell->compositor, target_seat_name);
    pepper_seat_add_input_device(seat, device);
}

static void
default_pointer_grab_motion(pepper_pointer_t *pointer, void *data, uint32_t time, double x, double y)
{
    double               vx, vy;
    pepper_compositor_t *compositor = pepper_pointer_get_compositor(pointer);
    pepper_view_t       *view = pepper_compositor_pick_view(compositor, x, y, &vx, &vy);
    pepper_view_t       *focus = pepper_pointer_get_focus(pointer);

    if (focus != view)
    {
        pepper_pointer_send_leave(pointer, focus);
        pepper_pointer_set_focus(pointer, view);
        pepper_pointer_send_enter(pointer, view, vx, vy);
    }

    pepper_pointer_send_motion(pointer, view, time, vx, vy);
}

static void
default_pointer_grab_button(pepper_pointer_t *pointer, void *data,
                            uint32_t time, uint32_t button, uint32_t state)
{
    pepper_seat_t       *seat = pepper_pointer_get_seat(pointer);
    pepper_keyboard_t   *keyboard = pepper_seat_get_keyboard(seat);
    pepper_view_t       *pointer_focus = pepper_pointer_get_focus(pointer);

    if (keyboard && state == PEPPER_BUTTON_STATE_PRESSED)
    {
        pepper_view_t *keyboard_focus = pepper_keyboard_get_focus(keyboard);

        if (keyboard_focus != pointer_focus)
        {
            pepper_keyboard_send_leave(keyboard, keyboard_focus);
            pepper_keyboard_set_focus(keyboard, pointer_focus);
            pepper_keyboard_send_enter(keyboard, pointer_focus);
        }

        if (pointer_focus)
        {
            shell_seat_t       *shseat = data;
            desktop_shell_t    *shell = shseat->shell;
            pepper_surface_t   *surface = pepper_view_get_surface(pointer_focus);
            shell_surface_t    *shsurf = get_shsurf_from_surface(surface, shell);

            shell_surface_stack_top(shsurf, PEPPER_FALSE);
        }
    }

    pepper_pointer_send_button(pointer, pointer_focus, time, button, state);
}

static void
default_pointer_grab_axis(pepper_pointer_t *pointer, void *data,
                          uint32_t time, uint32_t axis, double value)
{
    pepper_pointer_send_axis(pointer, pepper_pointer_get_focus(pointer), time, axis, value);
}

static void
default_pointer_grab_cancel(pepper_pointer_t *pointer, void *data)
{
    /* Nothing to do.*/
}

static const pepper_pointer_grab_t default_pointer_grab =
{
    default_pointer_grab_motion,
    default_pointer_grab_button,
    default_pointer_grab_axis,
    default_pointer_grab_cancel,
};

static void
pointer_add_callback(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id,
                     void *info, void *data)
{
    pepper_pointer_t *pointer = info;
    pepper_pointer_set_grab(pointer, &default_pointer_grab, data);
}

static void
pointer_remove_callback(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id,
                        void *info, void *data)
{
    /* Nothing to do. */
}

static void
default_keyboard_grab_key(pepper_keyboard_t *keyboard, void *data,
                          uint32_t time, uint32_t key, uint32_t state)
{
    pepper_keyboard_send_key(keyboard, pepper_keyboard_get_focus(keyboard), time, key, state);
}

static void
default_keyboard_grab_modifiers(pepper_keyboard_t *keyboard, void *data, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    pepper_keyboard_send_modifiers(keyboard, pepper_keyboard_get_focus(keyboard),
                                   mods_depressed, mods_latched, mods_locked, group);
}

static void
default_keyboard_grab_cancel(pepper_keyboard_t *keyboard, void *data)
{
    /* Nothing to do. */
}

static const pepper_keyboard_grab_t default_keyboard_grab =
{
    default_keyboard_grab_key,
    default_keyboard_grab_modifiers,
    default_keyboard_grab_cancel,
};

static void
keyboard_add_callback(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id,
                     void *info, void *data)
{
    pepper_keyboard_t *keyboard = info;
    pepper_keyboard_set_grab(keyboard, &default_keyboard_grab, NULL);
}

static void
keyboard_remove_callback(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id,
                        void *info, void *data)
{
    /* Nothing to do. */
}

static void
touch_add_callback(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id,
                     void *info, void *data)
{
    /* TODO: */
}

static void
touch_remove_callback(pepper_event_listener_t *listener, pepper_object_t *object, uint32_t id,
                        void *info, void *data)
{
    /* TODO: */
}

static void
shell_add_seat(desktop_shell_t *shell, pepper_seat_t *seat)
{
    shell_seat_t            *shseat;

    pepper_list_for_each(shseat, &shell->shseat_list, link)
    {
        if (shseat->seat == seat)
            return ;
    }

    shseat = calloc(1, sizeof(shell_seat_t));
    if (!shseat)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return ;
    }

    shseat->seat  = seat;
    shseat->shell = shell;

    pepper_list_insert(&shell->shseat_list, &shseat->link);
    pepper_object_set_user_data((pepper_object_t *)seat, shell, shseat, NULL);

    shseat->pointer_add_listener =
        pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_POINTER_ADD,
                                         0, pointer_add_callback, shseat);

    shseat->pointer_remove_listener =
        pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_POINTER_REMOVE,
                                         0, pointer_remove_callback, shseat);

    shseat->keyboard_add_listener =
        pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_KEYBOARD_ADD,
                                         0, keyboard_add_callback, shseat);

    shseat->keyboard_remove_listener =
        pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_KEYBOARD_REMOVE,
                                         0, keyboard_remove_callback, shseat);

    shseat->touch_add_listener =
        pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_TOUCH_ADD,
                                         0, touch_add_callback, shseat);

    shseat->touch_remove_listener =
        pepper_object_add_event_listener((pepper_object_t *)seat, PEPPER_EVENT_SEAT_TOUCH_REMOVE,
                                         0, touch_remove_callback, shseat);
}

static void
shell_remove_seat(desktop_shell_t *shell, pepper_seat_t *seat)
{
    shell_seat_t            *shseat;

    pepper_list_for_each(shseat, &shell->shseat_list, link)
    {
        if (shseat->seat == seat)
        {
            pepper_list_remove(&shseat->link);
            free(shseat);
            return ;
        }
    }
}

static void
input_device_add_callback(pepper_event_listener_t    *listener,
                          pepper_object_t            *object,
                          uint32_t                    id,
                          void                       *info,
                          void                       *data)
{
    shell_add_input_device(data, info);
}

static void
seat_add_callback(pepper_event_listener_t    *listener,
                  pepper_object_t            *object,
                  uint32_t                    id,
                  void                       *info,
                  void                       *data)
{
    shell_add_seat(data, info);
}

static void
seat_remove_callback(pepper_event_listener_t    *listener,
                     pepper_object_t            *object,
                     uint32_t                    id,
                     void                       *info,
                     void                       *data)
{
    shell_remove_seat(data, info);
}

static void
init_listeners(desktop_shell_t *shell)
{
    pepper_object_t *compositor = (pepper_object_t *)shell->compositor;

    /* input_device_add */
    shell->input_device_add_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
                                         0, input_device_add_callback, shell);

    shell->seat_add_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
                                         0, seat_add_callback, shell);

    shell->seat_remove_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
                                         0, seat_remove_callback, shell);
}

static void
init_input(desktop_shell_t *shell)
{
    pepper_list_t *l;
    const pepper_list_t *input_device_list =
        pepper_compositor_get_input_device_list(shell->compositor);

    pepper_list_for_each_list(l, input_device_list)
        shell_add_input_device(shell, l->item);
}

PEPPER_API pepper_bool_t
pepper_desktop_shell_init(pepper_compositor_t *compositor)
{
    desktop_shell_t *shell;

    shell = calloc(1, sizeof(desktop_shell_t));
    if (!shell)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return PEPPER_FALSE;
    }

    shell->compositor = compositor;

    pepper_list_init(&shell->shell_client_list);
    pepper_list_init(&shell->shell_surface_list);
    pepper_list_init(&shell->shseat_list);

    if (!init_wl_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        free(shell);
        return PEPPER_FALSE;
    }

    if (!init_xdg_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        fini_wl_shell(shell);
        free(shell);
        return PEPPER_FALSE;
    }

    init_listeners(shell);
    init_input(shell);

    return PEPPER_TRUE;
}
