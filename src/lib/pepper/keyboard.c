#include "pepper-internal.h"

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_impl =
{
    keyboard_release,
};

static void
keyboard_handle_focus_destroy(pepper_object_t *object, void *data)
{
    pepper_keyboard_t *keyboard = (pepper_keyboard_t *)object;

    if (keyboard->grab)
        keyboard->grab->cancel(keyboard, keyboard->data);
}

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat)
{
    pepper_keyboard_t *keyboard =
        (pepper_keyboard_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_keyboard_t));

    PEPPER_CHECK(keyboard, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&keyboard->input, seat, &keyboard->base, keyboard_handle_focus_destroy);
    return keyboard;
}

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard)
{
    if (keyboard->grab)
        keyboard->grab->cancel(keyboard, keyboard->data);

    pepper_input_fini(&keyboard->input);
    free(keyboard);
}

void
pepper_keyboard_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);

    if (!seat->keyboard)
        return;

    pepper_input_bind_resource(&seat->keyboard->input, client, wl_resource_get_version(resource),
                               id, &wl_keyboard_interface, &keyboard_impl, seat->keyboard);
}

PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *focus)
{
    pepper_keyboard_send_leave(keyboard);
    pepper_input_set_focus(&keyboard->input, focus);
    pepper_keyboard_send_enter(keyboard);
}

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard)
{
    return keyboard->input.focus;
}

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
            wl_keyboard_send_leave(resource, serial, keyboard->input.focus->surface->resource);
    }
}

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
        {
            /* TODO: Send currently pressed keys. */
            wl_keyboard_send_enter(resource, serial, keyboard->input.focus->surface->resource,
                                   NULL);
        }
    }
}

PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, uint32_t time, uint32_t key, uint32_t state)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
            wl_keyboard_send_key(resource, serial, time, key, state);
    }
}

PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard, uint32_t depressed, uint32_t latched,
                               uint32_t locked, uint32_t group)
{
    if (!wl_list_empty(&keyboard->input.focus_resource_list))
    {
        struct wl_resource *resource;
        uint32_t serial = wl_display_next_serial(keyboard->input.seat->compositor->display);

        wl_resource_for_each(resource, &keyboard->input.focus_resource_list)
            wl_keyboard_send_modifiers(resource, serial, depressed, latched, locked, group);
    }
}

PEPPER_API void
pepper_keyboard_start_grab(pepper_keyboard_t *keyboard,
                           const pepper_keyboard_grab_t *grab, void *data)
{
    keyboard->grab = grab;
    keyboard->data = data;
}

PEPPER_API void
pepper_keyboard_end_grab(pepper_keyboard_t *keyboard)
{
    /* TODO: switch back to default grab. */
    keyboard->grab = NULL;
}
