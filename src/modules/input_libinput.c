#include <libinput.h>
#include <libudev.h>

#include "common.h"
#include "pepper_internal.h"

static int
get_event_fd(void *data)
{
    struct libinput *li = (struct libinput *)data;
    return libinput_get_fd(li);
}

static void
dispatch_events(void *data)
{
    struct libinput *li = (struct libinput *)data;
    libinput_dispatch(li);
    return;
}

static void
set_input_event_data_pointer(pepper_input_event_t *event, struct libinput_event *e, int type)
{
    struct libinput_event_pointer *li_event = libinput_event_get_pointer_event(e);

    event->type = type;
    event->data.pointer.button = libinput_event_pointer_get_button(li_event);
    event->data.pointer.time = libinput_event_pointer_get_time(li_event);
    event->data.pointer.x = libinput_event_pointer_get_dx(li_event);
    event->data.pointer.y = libinput_event_pointer_get_dy(li_event);

    return;
}

static void
set_input_event_data_keyboard(pepper_input_event_t *event, struct libinput_event *e, int type)
{
    struct libinput_event_keyboard *li_event = libinput_event_get_keyboard_event(e);

    event->type = type;
    event->data.keyboard.key = libinput_event_keyboard_get_key(li_event);
    event->data.keyboard.time = libinput_event_keyboard_get_time(li_event);

    return;
}

static void
set_input_event_data_touch(pepper_input_event_t *event, struct libinput_event *e, int type)
{
    struct libinput_event_touch *li_event = libinput_event_get_touch_event(e);

    event->type = type;
    event->data.touch.index = libinput_event_touch_get_slot(li_event);
    event->data.touch.time = libinput_event_touch_get_time(li_event);
    event->data.touch.x = libinput_event_touch_get_x(li_event);
    event->data.touch.y = libinput_event_touch_get_y(li_event);

    return;
}

static int
get_next_event(pepper_input_event_t *event, void *data)
{
    struct libinput             *li;
    struct libinput_event       *li_event;
    enum libinput_event_type    type;

    if (!event)
        return -1;

    li = (struct libinput *)data;
    li_event = libinput_get_event(li);
    if (!li_event)
        return -1;

    type = libinput_event_get_type(li_event);
    switch (type)
    {
    case LIBINPUT_EVENT_POINTER_MOTION:
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
    case LIBINPUT_EVENT_POINTER_BUTTON:
    case LIBINPUT_EVENT_POINTER_AXIS:
        set_input_event_data_pointer(event, li_event, type);
        break;
    case LIBINPUT_EVENT_KEYBOARD_KEY:
        set_input_event_data_keyboard(event, li_event, type);
        break;
    case LIBINPUT_EVENT_TOUCH_DOWN:
    case LIBINPUT_EVENT_TOUCH_MOTION:
    case LIBINPUT_EVENT_TOUCH_UP:
    case LIBINPUT_EVENT_TOUCH_FRAME:
        set_input_event_data_touch(event, li_event, type);
        break;
    default:
        PEPPER_ERROR("%s Undefined event type!!\n", __FUNCTION__);
        return -1;
    }

    libinput_event_destroy(li_event);

    return 0;
};

static int
open_restricted(const char *path, int flags, void *user_data)
{
    /* TODO: */
}

static void
close_restricted(int fd, void *user_data)
{
    /* TODO: */
}

const struct libinput_interface libinput_interface =
{
    open_restricted,
    close_restricted
};

int
module_init(pepper_compositor_t *compositor)
{
    pepper_input_module_interface_t *interface;

    struct udev     *udev;
    struct libinput *li;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    udev = udev_new();
    if (!udev)
    {
        PEPPER_ERROR("%s udev_new failed\n", __FUNCTION__);
        goto err;
    }

    li = libinput_udev_create_context(&libinput_interface, NULL/* user_data */, udev);
    if (!li)
    {
        PEPPER_ERROR("%s libinput_udev_create_context failed\n", __FUNCTION__);
        goto err;
    }

    compositor->input_module_data = li;
    interface = &compositor->input_module_interface;
    interface->get_event_fd = get_event_fd;
    interface->dispatch_events = dispatch_events;
    interface->get_next_event = get_next_event;

    return 0;

err:
    if (li)
        libinput_unref(li);
    if (udev)
        udev_unref(udev);

    return -1;
}
