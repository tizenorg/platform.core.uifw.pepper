#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libinput-internal.h"

static void
libinput_seat_destroy(void *data)
{
    libinput_seat_t *seat = (libinput_seat_t *)data;

    wl_list_remove(&seat->link);

    if (seat->name)
        free(seat->name);

    free(seat);
}

static void
libinput_seat_add_capabilities_listener(void *data, struct wl_listener *listener)
{
    libinput_seat_t *seat = (libinput_seat_t *)data;
    wl_signal_add(&seat->capabilities_signal, listener);
}

static void
libinput_seat_add_name_listener(void *data, struct wl_listener *listener)
{
    libinput_seat_t *seat = (libinput_seat_t *)data;
    wl_signal_add(&seat->name_signal, listener);
}

static uint32_t
libinput_seat_get_capabilities(void *data)
{
    libinput_seat_t *seat = (libinput_seat_t *)data;
    return seat->caps;
}

static const char *
libinput_seat_get_name(void *data)
{
    libinput_seat_t *seat = (libinput_seat_t *)data;
    return seat->name;
}

static const pepper_seat_interface_t libinput_seat_interface =
{
    libinput_seat_destroy,
    libinput_seat_add_capabilities_listener,
    libinput_seat_add_name_listener,
    libinput_seat_get_capabilities,
    libinput_seat_get_name,
};

libinput_seat_t *
libinput_seat_create(pepper_libinput_t *input)
{
    libinput_seat_t *seat;

    seat = (libinput_seat_t *)calloc(1, sizeof(libinput_seat_t));
    if (!seat)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    wl_signal_init(&seat->capabilities_signal);
    wl_signal_init(&seat->name_signal);

    seat->input = input;
    seat->base = pepper_compositor_add_seat(input->compositor,
                                            &libinput_seat_interface, seat);
    if (!seat->base)
    {
        PEPPER_ERROR("Failed to create pepper_seat in %s\n", __FUNCTION__);
        goto error;
    }

    return seat;

error:
    if (seat)
        free(seat);

    return NULL;
}

static int
libinput_open_restricted(const char *path, int flags, void *user_data)
{
    int         fd;
    struct stat s;

    fd = open(path, flags | O_CLOEXEC);
    if (fd < 0)
    {
        PEPPER_ERROR("Failed to open file[%s] in %s\n", path, __FUNCTION__);
        return -1;
    }

    if (fstat(fd, &s) < 0)
    {
        PEPPER_ERROR("Failed to get file[%s] status in %s\n", path, __FUNCTION__);
        close(fd);
        return -1;
    }

    return fd;
}

static void
libinput_close_restricted(int fd, void *user_data)
{
    close(fd);
}

const struct libinput_interface libinput_interface =
{
    libinput_open_restricted,
    libinput_close_restricted,
};

static uint32_t
get_capabilities(struct libinput_device *device)
{
    uint32_t caps = 0;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER))
        caps |= WL_SEAT_CAPABILITY_POINTER;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD))
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH))
        caps |= WL_SEAT_CAPABILITY_TOUCH;

    return caps;
}

static void
device_added(pepper_libinput_t *input, struct libinput_device *libinput_device)
{
    struct libinput_seat   *libinput_seat;
    const char             *seat_name;
    libinput_seat_t        *tmp, *seat = NULL;
    uint32_t                caps;

    libinput_seat = libinput_device_get_seat(libinput_device);
    seat_name = libinput_seat_get_logical_name(libinput_seat);

    wl_list_for_each(tmp, &input->seat_list, link)
    {
        if (strcmp(tmp->name, seat_name) == 0)
        {
            seat = tmp;
            break;
        }
    }

    if (!seat)
    {
        seat = libinput_seat_create(input);
        if (!seat)
        {
            PEPPER_ERROR("Failed to create libinput_seat in %s\n", __FUNCTION__);
            return;
        }

        wl_list_insert(&input->seat_list, &seat->link);
    }

    libinput_device_set_user_data(libinput_device, seat);

    /* TODO
     *      check tab count
     *      check calibration
     */

    caps = get_capabilities(libinput_device);
    if (seat->caps != caps)
    {
        seat->caps |= caps;
        wl_signal_emit(&seat->capabilities_signal, seat);
    }

    if (!seat->name)
    {
        seat->name = strdup(seat_name);
        wl_signal_emit(&seat->name_signal, seat);
    }
}

static void
device_removed(pepper_libinput_t *input, struct libinput_device *libinput_device)
{
    libinput_seat_t    *seat;
    uint32_t            caps;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);
    caps = get_capabilities(libinput_device);

    if (seat->caps != caps)
    {
        seat->caps = caps;
        wl_signal_emit(&seat->capabilities_signal, seat);
    }
}

static void
pointer_motion(struct libinput_device *libinput_device,
               struct libinput_event_pointer *pointer_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_POINTER_MOTION;
    event.time = libinput_event_pointer_get_time(pointer_event);
    event.serial = 0;
    event.index = 0;
    event.state = 0;
    event.value = 0;
    event.x = seat->pointer_x_last = wl_fixed_from_double(
                                        libinput_event_pointer_get_dx(pointer_event));
    event.y = seat->pointer_y_last = wl_fixed_from_double(
                                        libinput_event_pointer_get_dy(pointer_event));

    pepper_seat_handle_event(seat->base, &event);
}

static void
pointer_motion_absolute(struct libinput_device *libinput_device,
                        struct libinput_event_pointer *pointer_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    /* TODO */

    pepper_seat_handle_event(seat->base, &event);
}

static void
pointer_button(struct libinput_device *libinput_device,
               struct libinput_event_pointer *pointer_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_POINTER_BUTTON;
    event.time = libinput_event_pointer_get_time(pointer_event);
    event.serial = 0;
    event.index = libinput_event_pointer_get_button(pointer_event);
    event.state = libinput_event_pointer_get_button_state(pointer_event);
    event.value = 0;
    event.x = seat->pointer_x_last;
    event.y = seat->pointer_y_last;

    pepper_seat_handle_event(seat->base, &event);
}

static void
pointer_axis(struct libinput_device *libinput_device,
             struct libinput_event_pointer *pointer_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_POINTER_AXIS;
    event.time = libinput_event_pointer_get_time(pointer_event);
    event.serial = 0;

    if (libinput_event_pointer_has_axis(pointer_event,
                                        LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL) != 0)
    {
        event.index = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL;
    }
    else if (libinput_event_pointer_has_axis(pointer_event,
                                             LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL) != 0)
    {
        event.index = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL;
    }

    event.state = 0;
    event.value = wl_fixed_from_double(libinput_event_pointer_get_axis_value(pointer_event,
                                                                             event.index));
    event.x = seat->pointer_x_last;
    event.y = seat->pointer_y_last;

    /* TODO: check axis_source */

    pepper_seat_handle_event(seat->base, &event);
}

static void
keyboard_key(struct libinput_device *libinput_device,
             struct libinput_event_keyboard *keyboard_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_KEYBOARD_KEY;
    event.time = libinput_event_keyboard_get_time(keyboard_event);
    event.serial = 0;
    event.index = libinput_event_keyboard_get_key(keyboard_event);
    event.state = libinput_event_keyboard_get_key_state(keyboard_event);
    event.value = 0;
    event.x = 0;
    event.y = 0;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_down(struct libinput_device *libinput_device,
           struct libinput_event_touch *touch_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;
    uint32_t                width, height;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_TOUCH_DOWN;
    event.time = libinput_event_touch_get_time(touch_event);
    event.serial = 0;
    event.index = libinput_event_touch_get_seat_slot(touch_event);  /* XXX */
    event.value = 0;
    event.state = 0;

    width = 1280; /* FIXME: get width from output */
    event.x = wl_fixed_from_double(
                    libinput_event_touch_get_x_transformed(touch_event, width));
    seat->touch_x_last = event.x;

    height = 720; /* FIXME: get height from output */
    event.y = wl_fixed_from_double(
                    libinput_event_touch_get_y_transformed(touch_event, height));
    seat->touch_y_last = event.y;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_up(struct libinput_device *libinput_device,
         struct libinput_event_touch *touch_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_TOUCH_UP;
    event.time = libinput_event_touch_get_time(touch_event);
    event.serial = 0;
    event.index = libinput_event_touch_get_seat_slot(touch_event);  /* XXX */
    event.value = 0;
    event.state = 0;
    event.x = seat->touch_x_last;
    event.y = seat->touch_y_last;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_motion(struct libinput_device *libinput_device,
             struct libinput_event_touch *touch_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;
    uint32_t                width, height;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);


    event.type = PEPPER_INPUT_EVENT_TOUCH_MOTION;
    event.time = libinput_event_touch_get_time(touch_event);
    event.serial = 0;
    event.index = libinput_event_touch_get_seat_slot(touch_event);  /* XXX */
    event.value = 0;
    event.state = 0;

    width = 1280; /* FIXME: get width from output */
    event.x = wl_fixed_from_double(
                    libinput_event_touch_get_x_transformed(touch_event, width));
    seat->touch_x_last = event.x;

    height = 720; /* FIXME: get height from output */
    event.y = wl_fixed_from_double(
                    libinput_event_touch_get_y_transformed(touch_event, height));
    seat->touch_y_last = event.y;

    pepper_seat_handle_event(seat->base, &event);
}

static void
touch_frame(struct libinput_device *libinput_device,
            struct libinput_event_touch *touch_event)
{
    libinput_seat_t        *seat;
    pepper_input_event_t    event;

    seat = (libinput_seat_t *)libinput_device_get_user_data(libinput_device);

    event.type = PEPPER_INPUT_EVENT_TOUCH_FRAME;
    pepper_seat_handle_event(seat->base, &event);
}

static void
dispatch_event(struct libinput_event *event)
{
    struct libinput         *libinput = libinput_event_get_context(event);
    struct libinput_device  *libinput_device = libinput_event_get_device(event);

    switch (libinput_event_get_type(event))
    {
    case LIBINPUT_EVENT_DEVICE_ADDED:
        device_added((pepper_libinput_t *)libinput_get_user_data(libinput),
                     libinput_device);
        break;
    case LIBINPUT_EVENT_DEVICE_REMOVED:
        device_removed((pepper_libinput_t *)libinput_get_user_data(libinput),
                       libinput_device);
        break;
    case LIBINPUT_EVENT_POINTER_MOTION:
        pointer_motion(libinput_device, libinput_event_get_pointer_event(event));
        break;
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        pointer_motion_absolute(libinput_device,
                                libinput_event_get_pointer_event(event));
        break;
    case LIBINPUT_EVENT_POINTER_BUTTON:
        pointer_button(libinput_device, libinput_event_get_pointer_event(event));
        break;
    case LIBINPUT_EVENT_POINTER_AXIS:
        pointer_axis(libinput_device, libinput_event_get_pointer_event(event));
        break;
    case LIBINPUT_EVENT_KEYBOARD_KEY:
        keyboard_key(libinput_device, libinput_event_get_keyboard_event(event));
        break;
    case LIBINPUT_EVENT_TOUCH_DOWN:
        touch_down(libinput_device, libinput_event_get_touch_event(event));
        break;
    case LIBINPUT_EVENT_TOUCH_UP:
        touch_up(libinput_device, libinput_event_get_touch_event(event));
        break;
    case LIBINPUT_EVENT_TOUCH_MOTION:
        touch_motion(libinput_device, libinput_event_get_touch_event(event));
        break;
    case LIBINPUT_EVENT_TOUCH_FRAME:
        touch_frame(libinput_device, libinput_event_get_touch_event(event));
        break;
    default:
        break;
    }
}

static int
dispatch_events(pepper_libinput_t *input)
{
    struct libinput_event  *event;

    if (libinput_dispatch(input->libinput) != 0)
    {
        PEPPER_ERROR("Failed to dispatch libinput events in %s\n", __FUNCTION__);
        return -1;
    }

    while ((event = libinput_get_event(input->libinput)))
    {
        dispatch_event(event);
        libinput_event_destroy(event);
    }

    return 0;
}

static int
handle_libinput_events(int fd, uint32_t mask, void *data)
{
    pepper_libinput_t *input = (pepper_libinput_t *)data;
    return dispatch_events(input);
}

PEPPER_API pepper_libinput_t *
pepper_libinput_create(pepper_compositor_t *compositor, struct udev *udev)
{
    struct wl_display      *display;
    struct wl_event_loop   *loop;
    pepper_libinput_t      *input;

    input = (pepper_libinput_t *)calloc(1, sizeof(pepper_libinput_t));
    if (!input)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    input->compositor = compositor;
    input->udev = udev;

    wl_list_init(&input->seat_list);

    input->libinput = libinput_udev_create_context(&libinput_interface, input, input->udev);
    if (!input->libinput)
    {
        PEPPER_ERROR("Failed to initialize libinput in %s\n", __FUNCTION__);
        goto error;
    }
    libinput_ref(input->libinput);

    if (libinput_udev_assign_seat(input->libinput, "seat0"/* FIXME */) != 0)
    {
        PEPPER_ERROR("Failed to assign seat in %s\n", __FUNCTION__);
        goto error;
    }

    dispatch_events(input);   /* FIXME */

    /* add libinput_fd to main loop */
    display = pepper_compositor_get_display(compositor);
    loop = wl_display_get_event_loop(display);
    input->libinput_fd = libinput_get_fd(input->libinput);
    input->libinput_event_source = wl_event_loop_add_fd(loop, input->libinput_fd,
                                                        WL_EVENT_READABLE,
                                                        handle_libinput_events, input);
    if (!input->libinput_event_source)
    {
        PEPPER_ERROR("Failed to add libinput fd to the main loop in %s\n", __FUNCTION__);
        goto error;
    }

    return input;

error:
    if (input)
        pepper_libinput_destroy(input);

    return NULL;
}

PEPPER_API void
pepper_libinput_destroy(pepper_libinput_t *input)
{
    libinput_seat_t *seat, *tmp;

    wl_list_for_each_safe(seat, tmp, &input->seat_list, link)
        libinput_seat_destroy(seat);

    if (input->libinput)
        libinput_unref(input->libinput);

    if (input->libinput_event_source)
        wl_event_source_remove(input->libinput_event_source);

    free(input);
}
