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

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat)
{
    pepper_keyboard_t *keyboard =
        (pepper_keyboard_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_keyboard_t));

    PEPPER_CHECK(keyboard, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&keyboard->input, seat);
    return keyboard;
}

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard)
{
    pepper_input_fini(&keyboard->input);
    free(keyboard);
}

void
pepper_keyboard_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);

    if (seat->keyboard)
        return;

    pepper_input_bind_resource(&seat->keyboard->input, client, wl_resource_get_version(resource),
                               id, &wl_keyboard_interface, &keyboard_impl, seat->keyboard);
}

PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *view)
{
    /* TODO: */
}

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard)
{
    /* TODO: */
    return NULL;
}

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard, pepper_view_t *target_view)
{
    /* TODO: */
}

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard, pepper_view_t *target_view)
{
    /* TODO: */
}
