#include "wayland-internal.h"

static void
pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    wayland_seat_t  *seat = (wayland_seat_t *)seat;

    seat->pointer_x_last = surface_x;
    seat->pointer_y_last = surface_y;

    /* TODO */
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
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_POINTER_MOTION;
    event.time = time;
    event.serial = 0;
    event.index = 0;
    event.state = 0;
    event.value = 0;
    event.x = seat->pointer_x_last = surface_x;
    event.y = seat->pointer_y_last = surface_y;

    pepper_seat_handle_event(seat->base, &event);
}

static void
pointer_handle_button(void *data, struct wl_pointer *pointer,
                     uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_POINTER_BUTTON;
    event.time = time;
    event.serial = serial; /* FIXME */
    event.index = button;
    event.state = state;
    event.value = 0;
    event.x = seat->pointer_x_last;
    event.y = seat->pointer_y_last;

    pepper_seat_handle_event(seat->base, &event);
}

static void
pointer_handle_axis(void *data, struct wl_pointer *pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_POINTER_AXIS;
    event.time = time;
    event.serial = 0;
    event.index = axis;
    event.value = value;
    event.state = 0;
    event.x = 0;
    event.y = 0;

    pepper_seat_handle_event(seat->base, &event);
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
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_KEYBOARD_KEY;
    event.time = time;
    event.serial = serial;  /* FIXME */
    event.index = key;
    event.value = 0;
    event.state = state;
    event.x = 0;
    event.y = 0;

    pepper_seat_handle_event(seat->base, &event);
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
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_TOUCH_DOWN;
    event.time = time;
    event.serial = serial;  /* FIXME */
    event.index = id;
    event.value = 0;
    event.state = 0;
    event.x = seat->touch_x_last = x;
    event.y = seat->touch_y_last = y;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_handle_up(void *data, struct wl_touch *touch,
                uint32_t serial, uint32_t time, int32_t id)
{
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_TOUCH_UP;
    event.time = time;
    event.serial = serial;  /* FIXME */
    event.index = id;
    event.value = 0;
    event.state = 0;
    event.x = seat->touch_x_last;
    event.y = seat->touch_y_last;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_handle_motion(void *data, struct wl_touch *touch,
                    uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_TOUCH_MOTION;
    event.time = time;
    event.serial = 0;
    event.index = id;
    event.value = 0;
    event.state = 0;
    event.x = seat->touch_x_last = x;
    event.y = seat->touch_y_last = y;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_handle_frame(void *data, struct wl_touch *touch)
{
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_TOUCH_FRAME;
    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_handle_cancel(void *data, struct wl_touch *touch)
{
    wayland_seat_t         *seat = (wayland_seat_t *)seat;
    pepper_input_event_t    event;

    event.type = PEPPER_INPUT_EVENT_TOUCH_CANCEL;
    pepper_seat_handle_event(seat->base, &event);
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
seat_handle_caps(void *data, struct wl_seat *s, enum wl_seat_capability caps)
{
    wayland_seat_t  *seat = (wayland_seat_t *)data;

    if (seat->seat != s) /* FIXME */
        return;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && (!seat->pointer))
    {
        seat->pointer = wl_seat_get_pointer(seat->seat);
        if (seat->pointer)
            wl_pointer_add_listener(seat->pointer, &pointer_listener, seat);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && (seat->pointer))
    {
        wl_pointer_release(seat->pointer);
        seat->pointer = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && (!seat->keyboard))
    {
        seat->keyboard = wl_seat_get_keyboard(seat->seat);
        if (seat->keyboard)
            wl_keyboard_add_listener(seat->keyboard, &keyboard_listener, seat);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && (seat->keyboard))
    {
        wl_keyboard_release(seat->keyboard);
        seat->keyboard = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && (!seat->touch))
    {
        seat->touch = wl_seat_get_touch(seat->seat);
        if (seat->touch)
            wl_touch_add_listener(seat->touch, &touch_listener, seat);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && (seat->touch))
    {
        wl_touch_release(seat->touch);
        seat->touch = NULL;
    }

    seat->caps = caps;
    wl_signal_emit(&seat->capability_signal, seat);
}

static void
seat_handle_name(void *data, struct wl_seat *s, const char *name)
{
    wayland_seat_t  *seat = (wayland_seat_t *)data;

    if (seat->seat != s) /* FIXME */
        return;

    seat->name = pepper_string_copy(name);
    wl_signal_emit(&seat->name_signal, seat);
}

static const struct wl_seat_listener seat_listener =
{
    seat_handle_caps,
    seat_handle_name,
};

static void
wayland_seat_destroy(void *data)
{
    /* TODO: */
}

static void
wayland_seat_add_capability_listener(void *data, struct wl_listener *listener)
{
    wayland_seat_t *seat = (wayland_seat_t *)data;
    wl_signal_add(&seat->capability_signal, listener);
}

static void
wayland_seat_add_name_listener(void *data, struct wl_listener *listener)
{
    wayland_seat_t *seat = (wayland_seat_t *)data;
    wl_signal_add(&seat->name_signal, listener);
}

static uint32_t
wayland_seat_get_capabilities(void *data)
{
    wayland_seat_t *seat = (wayland_seat_t *)data;
    return seat->caps;
}

static const char *
wayland_seat_get_name(void *data)
{
    wayland_seat_t *seat = (wayland_seat_t *)data;
    return seat->name;
}

static const pepper_seat_interface_t wayland_seat_interface =
{
    wayland_seat_destroy,
    wayland_seat_add_capability_listener,
    wayland_seat_add_name_listener,
    wayland_seat_get_capabilities,
    wayland_seat_get_name,
};

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version)
{
    wayland_seat_t  *seat;

    seat = (wayland_seat_t *)pepper_calloc(1, sizeof(wayland_seat_t));
    if (!seat)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return;
    }

    seat->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
    wl_seat_add_listener(seat->seat, &seat_listener, seat);

    wl_signal_init(&seat->capability_signal);
    wl_signal_init(&seat->name_signal);

    seat->base = pepper_compositor_add_seat(conn->pepper, &wayland_seat_interface, seat);
    seat->id = name;

    wl_list_init(&seat->link);
    wl_list_insert(&conn->seat_list, &seat->link);
}
