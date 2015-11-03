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
                               pepper_list_t *property_list)
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

        pepper_list_insert(property_list, &property->link);
        e = udev_list_entry_get_next(e);
    }
}

static const char *
li_device_get_property(void *dev, const char *key)
{
    li_device_t            *device = (li_device_t *)dev;
    li_device_property_t   *property;

    pepper_list_for_each(property, &device->property_list, link)
    {
        if (!strcmp(property->key, key))
            return property->data;
    }

    return NULL;
}

static const pepper_input_device_backend_t li_device_backend =
{
    li_device_get_property,
};

static void
device_added(pepper_libinput_t *input, struct libinput_device *libinput_device)
{
    pepper_list_t       property_list;
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

    device->base = pepper_input_device_create(input->compositor, caps,
                                              &li_device_backend, device);
    if (!device->base)
    {
        PEPPER_ERROR("Failed to create pepper input device\n");
        /* TODO: error handling */
        return;
    }

    pepper_list_init(&property_list);                                   /* FIXME */
    libinput_device_get_properties(libinput_device, &property_list);    /* FIXME */
    pepper_list_init(&device->property_list);                           /* FIXME */
    pepper_list_insert_list(&device->property_list, &property_list);    /* FIXME */

    pepper_list_insert(&input->device_list, &device->link);
    libinput_device_set_user_data(libinput_device, device);
}

static void
clear_property_list(pepper_list_t *list)
{
    li_device_property_t *property, *tmp;

    if (pepper_list_empty(list))
        return;

    pepper_list_for_each_safe(property, tmp, list, link)
    {
        pepper_list_remove(&property->link);

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
    pepper_list_remove(&device->link);
    clear_property_list(&device->property_list);

    if (device->base)
        pepper_input_device_destroy(device->base);

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
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_pointer_get_time(pointer_event);
    event.x = libinput_event_pointer_get_dx(pointer_event);
    event.y = libinput_event_pointer_get_dy(pointer_event);

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION, &event);
}

static void
pointer_motion_absolute(struct libinput_device *libinput_device,
                        struct libinput_event_pointer *pointer_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_pointer_get_time(pointer_event);
    event.x = libinput_event_pointer_get_absolute_x_transformed(pointer_event, 1);
    event.y = libinput_event_pointer_get_absolute_y_transformed(pointer_event, 1);

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE, &event);
}

static void
pointer_button(struct libinput_device *libinput_device,
               struct libinput_event_pointer *pointer_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_pointer_get_time(pointer_event);
    event.button = libinput_event_pointer_get_button(pointer_event);
    event.state = libinput_event_pointer_get_button_state(pointer_event);

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON, &event);
}

static void
pointer_axis(struct libinput_device *libinput_device,
             struct libinput_event_pointer *pointer_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_pointer_get_time(pointer_event);

    if (libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL))
    {
        event.axis = PEPPER_POINTER_AXIS_VERTICAL;
        event.value = libinput_event_pointer_get_axis_value(pointer_event,
                                                            LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
        pepper_object_emit_event((pepper_object_t *)device->base,
                                 PEPPER_EVENT_INPUT_DEVICE_POINTER_AXIS, &event);
    }
    else if (libinput_event_pointer_has_axis(pointer_event, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL))
    {
        event.axis = PEPPER_POINTER_AXIS_HORIZONTAL;
        event.value = libinput_event_pointer_get_axis_value(pointer_event,
                                                            LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
        pepper_object_emit_event((pepper_object_t *)device->base,
                                 PEPPER_EVENT_INPUT_DEVICE_POINTER_AXIS, &event);
    }
}

static void
keyboard_key(struct libinput_device *libinput_device,
             struct libinput_event_keyboard *keyboard_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_keyboard_get_time(keyboard_event);
    event.key = libinput_event_keyboard_get_key(keyboard_event);

    if (libinput_event_keyboard_get_key_state(keyboard_event) == LIBINPUT_KEY_STATE_RELEASED)
        event.state = PEPPER_KEY_STATE_RELEASED;
    else
        event.state = PEPPER_KEY_STATE_PRESSED;

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY, &event);
}

static void
touch_down(struct libinput_device *libinput_device,
           struct libinput_event_touch *touch_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_touch_get_time(touch_event);
    event.slot = libinput_event_touch_get_seat_slot(touch_event);
    event.x = libinput_event_touch_get_x_transformed(touch_event, 1);
    event.y = libinput_event_touch_get_y_transformed(touch_event, 1);

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_TOUCH_DOWN, &event);
}

static void
touch_up(struct libinput_device *libinput_device,
         struct libinput_event_touch *touch_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_touch_get_time(touch_event);
    event.slot = libinput_event_touch_get_seat_slot(touch_event);

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_TOUCH_UP, &event);
}

static void
touch_motion(struct libinput_device *libinput_device,
             struct libinput_event_touch *touch_event)
{

    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_input_event_t    event;

    event.time = libinput_event_touch_get_time(touch_event);
    event.slot = libinput_event_touch_get_seat_slot(touch_event);
    event.x = libinput_event_touch_get_x_transformed(touch_event, 1);
    event.y = libinput_event_touch_get_y_transformed(touch_event, 1);

    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_TOUCH_MOTION, &event);
}

static void
touch_frame(struct libinput_device *libinput_device,
            struct libinput_event_touch *touch_event)
{
    li_device_t            *device = libinput_device_get_user_data(libinput_device);
    pepper_object_emit_event((pepper_object_t *)device->base,
                             PEPPER_EVENT_INPUT_DEVICE_TOUCH_FRAME, NULL);
}

static void
dispatch_event(struct libinput_event *event)
{
    struct libinput         *libinput = libinput_event_get_context(event);
    struct libinput_device  *libinput_device = libinput_event_get_device(event);

    switch (libinput_event_get_type(event))
    {
    case LIBINPUT_EVENT_DEVICE_ADDED:
        device_added((pepper_libinput_t *)libinput_get_user_data(libinput), libinput_device);
        break;
    case LIBINPUT_EVENT_DEVICE_REMOVED:
        device_removed((pepper_libinput_t *)libinput_get_user_data(libinput), libinput_device);
        break;
    case LIBINPUT_EVENT_POINTER_MOTION:
        pointer_motion(libinput_device, libinput_event_get_pointer_event(event));
        break;
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        pointer_motion_absolute(libinput_device, libinput_event_get_pointer_event(event));
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
    pepper_list_init(&input->device_list);

    input->libinput = libinput_udev_create_context(&libinput_interface, input, input->udev);
    if (!input->libinput)
    {
        PEPPER_ERROR("Failed to initialize libinput in %s\n", __FUNCTION__);
        goto error;
    }

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
    li_device_t *device, *tmp;

    pepper_list_for_each_safe(device, tmp, &input->device_list, link)
        li_device_destroy(device);

    if (input->libinput)
        libinput_unref(input->libinput);

    if (input->libinput_event_source)
        wl_event_source_remove(input->libinput_event_source);

    free(input);
}
