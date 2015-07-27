#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libinput-internal.h"

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
libinput_device_get_properties(struct libinput_device *libinput_device,
                               struct wl_list *property_list)
{
    /* get properties */
    struct udev_device     *udev_device = libinput_device_get_udev_device(libinput_device);
    struct udev_list_entry *e;

    e = udev_device_get_properties_list_entry(udev_device);
    while (e)
    {
        li_device_property_t *property;
        const char *name = udev_list_entry_get_name(e);
        const char *value = udev_list_entry_get_value(e);

        property = (li_device_property_t *)calloc(1, sizeof(li_device_property_t));
        if (!property)
            continue;

        property->key = strdup(name);
        property->data = strdup(value);

        wl_list_insert(property_list, &property->link);

        e = udev_list_entry_get_next(e);
    }
}

static void
device_added(pepper_libinput_t *input, struct libinput_device *libinput_device)
{
    struct wl_list      property_list;
    uint32_t            caps;
    li_device_t        *device;

    caps = get_capabilities(libinput_device);
    device = (li_device_t *)calloc(1, sizeof(li_device_t));
    if (!device)
    {
        PEPPER_ERROR("Failed to allocate memory\n");
        /* TODO: error handling */
        return;
    }

    device->input = input;
    device->caps = caps;


    if (caps & WL_SEAT_CAPABILITY_POINTER)
    {
        pepper_pointer_device_t *pointer_device;

        pointer_device = pepper_pointer_device_create(input->compositor);
        if (!pointer_device)
        {
            PEPPER_ERROR("Failed to create pepper pointer device\n");
            /* TODO: error handling */
            return;
        }
        device->pointer = pointer_device;
    }
    else if (caps & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        pepper_keyboard_device_t *keyboard_device;

        keyboard_device = pepper_keyboard_device_create(input->compositor);
        if (!keyboard_device)
        {
            PEPPER_ERROR("Failed to create pepper keyboard device\n");
            /* TODO: error handling */
            return;
        }
        device->keyboard = keyboard_device;
    }
    else if (caps & WL_SEAT_CAPABILITY_TOUCH)
    {
        pepper_touch_device_t *touch_device;

        touch_device = pepper_touch_device_create(input->compositor);
        if (!touch_device)
        {
            PEPPER_ERROR("Failed to create pepper touch device\n");
            /* TODO: error handling */
            return;
        }
        device->touch = touch_device;
    }

    wl_list_init(&property_list);                                       /* FIXME */
    libinput_device_get_properties(libinput_device, &property_list);    /* FIXME */
    wl_list_init(&device->property_list);                               /* FIXME */
    wl_list_insert_list(&device->property_list, &property_list);        /* FIXME */

    wl_list_insert(&input->device_list, &device->link);
    libinput_device_set_user_data(libinput_device, device);
}

static void
clear_property_list(struct wl_list *list)
{
    li_device_property_t   *property, *tmp;

    if (wl_list_empty(list))
        return;

    wl_list_for_each_safe(property, tmp, list, link)
    {
        wl_list_remove(&property->link);

        if (property->key)
            free(property->key);
        if (property->data)
            free(property->data);

        free(property);
    }
}

static void
li_device_destroy(li_device_t *device)
{
    wl_list_remove(&device->link);
    clear_property_list(&device->property_list);

    if (device->pointer)
        pepper_pointer_device_destroy(device->pointer);

    if (device->keyboard)
        pepper_keyboard_device_destroy(device->keyboard);

    if (device->touch)
        pepper_touch_device_destroy(device->touch);

    free(device);
}

static void
device_removed(pepper_libinput_t *input, struct libinput_device *libinput_device)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    li_device_destroy(device);
}

static void
pointer_motion(struct libinput_device *libinput_device,
               struct libinput_event_pointer *pointer_event)
{
    li_device_t    *device;
    wl_fixed_t      dx, dy;

    device = (li_device_t *)libinput_device_get_user_data(libinput_device);

    dx = wl_fixed_from_double(libinput_event_pointer_get_dx(pointer_event));
    dy = wl_fixed_from_double(libinput_event_pointer_get_dy(pointer_event));

    /* TODO */
}

static void
pointer_motion_absolute(struct libinput_device *libinput_device,
                        struct libinput_event_pointer *pointer_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    /* TODO */
}

static void
pointer_button(struct libinput_device *libinput_device,
               struct libinput_event_pointer *pointer_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    /* TODO */
}

static void
pointer_axis(struct libinput_device *libinput_device,
             struct libinput_event_pointer *pointer_event)
{
    int             axis = -1;
    wl_fixed_t      value;
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);

    if (libinput_event_pointer_has_axis(pointer_event,
                                        LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL) != 0)
        axis = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL; /* FIXME */
    else if (libinput_event_pointer_has_axis(pointer_event,
                                             LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL) != 0)
        axis = LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL; /* FIXME */

    if (axis < 0)
        return;

    value = wl_fixed_from_double(libinput_event_pointer_get_axis_value(pointer_event, axis));

    /* TODO */
}

static void
keyboard_key(struct libinput_device *libinput_device,
             struct libinput_event_keyboard *keyboard_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);

    /* TODO */
}

static void
touch_down(struct libinput_device *libinput_device,
           struct libinput_event_touch *touch_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    /* TODO */
}

static void
touch_up(struct libinput_device *libinput_device,
         struct libinput_event_touch *touch_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    /* TODO */
}

static void
touch_motion(struct libinput_device *libinput_device,
             struct libinput_event_touch *touch_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    /* TODO */
}

static void
touch_frame(struct libinput_device *libinput_device,
            struct libinput_event_touch *touch_event)
{
    li_device_t    *device;
    device = (li_device_t *)libinput_device_get_user_data(libinput_device);
    /* TODO */
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
    wl_list_init(&input->device_list);

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
    li_device_t  *device, *tmp;

    wl_list_for_each_safe(device, tmp, &input->device_list, link)
        li_device_destroy(device);

    if (input->libinput)
        libinput_unref(input->libinput);

    if (input->libinput_event_source)
        wl_event_source_remove(input->libinput_event_source);

    free(input);
}
