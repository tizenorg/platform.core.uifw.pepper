#include "wayland-internal.h"

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
pointer_handle_button(void *data, struct wl_pointer *pointer,
                     uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
pointer_handle_axis(void *data, struct wl_pointer *pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static const struct wl_pointer_listener pointer_listener =
{
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
};

static void
keyboard_handle_key_map(void *data, struct wl_keyboard *keyboard,
                        uint32_t format, int32_t fd, uint32_t size)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
                          uint32_t mods_locked, uint32_t group)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
keyboard_handle_repeat_info(void *data, struct wl_keyboard *keyboard,
                            int32_t rate, int32_t delay)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static const struct wl_keyboard_listener keyboard_listener =
{
    keyboard_handle_key_map,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
    keyboard_handle_repeat_info
};

static void
touch_handle_down(void *data, struct wl_touch *touch,
                  uint32_t serial, uint32_t time, struct wl_surface *surface,
                  int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
touch_handle_up(void *data, struct wl_touch *touch,
                uint32_t serial, uint32_t time, int32_t id)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
touch_handle_motion(void *data, struct wl_touch *touch,
                    uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
touch_handle_frame(void *data, struct wl_touch *touch)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static void
touch_handle_cancel(void *data, struct wl_touch *touch)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static const struct wl_touch_listener touch_listener =
{
    touch_handle_down,
    touch_handle_up,
    touch_handle_motion,
    touch_handle_frame,
    touch_handle_cancel,
};

static void
seat_handle_caps(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
    pepper_wayland_t *conn = data;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && (!conn->pointer))
    {
        conn->pointer = wl_seat_get_pointer(seat);

        if (conn->seat)
            wl_pointer_add_listener(conn->pointer, &pointer_listener, conn->seat);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && (conn->pointer))
    {
        wl_pointer_release(conn->pointer);
        conn->pointer = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && (!conn->keyboard))
    {
        conn->keyboard = wl_seat_get_keyboard(seat);

        if (conn->seat)
            wl_keyboard_add_listener(conn->keyboard, &keyboard_listener, conn->seat);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && (conn->keyboard))
    {
        wl_keyboard_release(conn->keyboard);
        conn->keyboard = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && (!conn->touch))
    {
        conn->touch = wl_seat_get_touch(seat);

        if (conn->seat)
            wl_touch_add_listener(conn->touch, &touch_listener, conn->seat);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && (conn->touch))
    {
        wl_touch_release(conn->touch);
        conn->touch = NULL;
    }
}

static void
seat_handle_name(void *data, struct wl_seat *seat, const char *name)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static const struct wl_seat_listener seat_listener =
{
    seat_handle_caps,
    seat_handle_name,
};

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version)
{
    conn->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    wl_seat_add_listener(conn->seat, &seat_listener, conn);
}
