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

static pepper_input_event_t *
create_pointer_event(struct libinput_event *li_event, int type)
{
    /* TODO: */
}

static pepper_input_event_t *
create_keyboard_event(struct libinput_event *li_event, int type)
{
    /* TODO: */
}

static pepper_input_event_t *
create_touch_event(struct libinput_event *li_event, int type)
{
    /* TODO: */
}

static pepper_input_event_t *
get_next_event(void *data)
{
    pepper_input_event_t        *event;
    struct libinput             *li = (struct libinput *)data;
    struct libinput_event       *li_event = libinput_get_event(li);
    enum libinput_event_type    type = libinput_event_get_type(li_event);

    switch (type)
    {
    case LIBINPUT_EVENT_POINTER_MOTION:
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
    case LIBINPUT_EVENT_POINTER_BUTTON:
    case LIBINPUT_EVENT_POINTER_AXIS:
        create_pointer_event(li_event, type);
        break;
    case LIBINPUT_EVENT_KEYBOARD_KEY:
        create_keyboard_event(li_event, type);
        break;
    case LIBINPUT_EVENT_TOUCH_DOWN:
    case LIBINPUT_EVENT_TOUCH_MOTION:
    case LIBINPUT_EVENT_TOUCH_UP:
    case LIBINPUT_EVENT_TOUCH_FRAME:
        create_touch_event(li_event, type);
        break;
    default:
        /* TODO: */
        break;
    }

    libinput_event_destroy(li_event);

    /* TODO: */

    return event;
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
