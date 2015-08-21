#include "pepper-internal.h"

typedef struct pepper_input_device_entry pepper_input_device_entry_t;

struct pepper_input_device_entry
{
    pepper_seat_t              *seat;
    pepper_input_device_t      *device;
    pepper_event_listener_t    *listener;
    pepper_list_t               link;
};

static void
unbind_resource(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial,
                   struct wl_resource *surface_resource, int32_t x, int32_t y)
{
    /* TODO */
}

static void
pointer_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_pointer_interface pointer_interface =
{
    pointer_set_cursor,
    pointer_release,
};

static void
seat_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    struct wl_resource *r;

    if (!seat->pointer.active)
        return;

    r = wl_resource_create(client, &wl_pointer_interface,
                           wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->pointer.resource_list, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &pointer_interface, &seat->pointer, unbind_resource);

    /* TODO */

}

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_interface =
{
    keyboard_release,
};

static void
seat_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    struct wl_resource *r;

    if (!seat->keyboard.active)
        return;

    r = wl_resource_create(client, &wl_keyboard_interface,
                           wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->keyboard.resource_list, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &keyboard_interface, &seat->keyboard, unbind_resource);

    /* TODO */

}

static void
touch_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_touch_interface touch_interface =
{
    touch_release,
};

static void
seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    struct wl_resource *r;

    if (!seat->touch.active)
        return;

    r = wl_resource_create(client, &wl_touch_interface, wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->touch.resource_list, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &touch_interface, &seat->touch, unbind_resource);

    /* TODO */

}

static const struct wl_seat_interface seat_interface = {
    seat_get_pointer,
    seat_get_keyboard,
    seat_get_touch,
};

static void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    pepper_seat_t *seat = (pepper_seat_t *)data;
    struct wl_resource  *resource;

    resource = wl_resource_create(client, &wl_seat_interface, version/*FIXME*/, id);
    wl_list_insert(&seat->resources, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, &seat_interface, data, unbind_resource);

    wl_seat_send_capabilities(resource, seat->caps);

    if ((seat->name) && (version >= WL_SEAT_NAME_SINCE_VERSION))
        wl_seat_send_name(resource, seat->name);
}

PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor,
                           const char *seat_name,
                           void *data)
{
    pepper_seat_t  *seat;

    seat = (pepper_seat_t *)pepper_object_alloc(PEPPER_OBJECT_SEAT, sizeof(pepper_seat_t));
    if (!seat)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    seat->compositor = compositor;
    seat->data = data;

    wl_list_init(&seat->resources);
    wl_list_init(&seat->link);
    wl_list_insert(&compositor->seat_list, &seat->link);

    if (seat_name)
        seat->name = strdup(seat_name);

    seat->global = wl_global_create(compositor->display, &wl_seat_interface, 4, seat, bind_seat);
    pepper_list_init(&seat->input_device_list);

    pepper_object_emit_event(&seat->compositor->base,
                             PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
                             seat);

    return seat;
}

PEPPER_API pepper_pointer_t *
pepper_seat_get_pointer(pepper_seat_t *seat)
{
    if (seat->pointer.active)
        return &seat->pointer;

    return NULL;
}

PEPPER_API pepper_keyboard_t *
pepper_seat_get_keyboard(pepper_seat_t *seat)
{
    if (seat->keyboard.active)
        return &seat->keyboard;

    return NULL;
}

PEPPER_API pepper_touch_t *
pepper_seat_get_touch(pepper_seat_t *seat)
{
    if (seat->touch.active)
        return &seat->touch;

    return NULL;
}

PEPPER_API const char *
pepper_seat_get_name(pepper_seat_t *seat)
{
    return seat->name;
}

PEPPER_API void
pepper_pointer_set_position(pepper_pointer_t *pointer, int32_t x, int32_t y)
{
    /* TODO */
}

PEPPER_API void
pepper_pointer_get_position(pepper_pointer_t *pointer, int32_t *x, int32_t *y)
{
    /* TODO */
}

PEPPER_API pepper_view_t *
pepper_pointer_get_focus_view(pepper_pointer_t *pointer)
{
    /* TODO */
    return NULL;
}

PEPPER_API void
pepper_pointer_set_focus_view(pepper_pointer_t *pointer, pepper_view_t *view)
{
    /* TODO */
}

PEPPER_API void
pepper_pointer_send_leave(pepper_pointer_t *pointer, pepper_view_t *target_view)
{
    /* TODO */
}

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, pepper_view_t *target_view)
{
    /* TODO */
}

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t     *pointer,
                           pepper_view_t        *target_view,
                           uint32_t              time,
                           int32_t               x,
                           int32_t               y)
{
    /* TODO */
}

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t     *pointer,
                           pepper_view_t        *target_view,
                           uint32_t              time,
                           uint32_t              button,
                           uint32_t              state)
{
    /* TODO */
}

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t   *pointer,
                         pepper_view_t      *target_view,
                         uint32_t            time,
                         uint32_t            axis,
                         uint32_t            amount)
{
    /* TODO */
}

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus_view(pepper_keyboard_t *keyboard)
{
    /* TODO */
    return NULL;
}

PEPPER_API void
pepper_keyboard_set_focus_view(pepper_keyboard_t *keyboard, pepper_view_t *view)
{
    /* TODO */
}

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard, pepper_view_t *target_view)
{
    /* TODO */
}

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard, pepper_view_t *target_view)
{
    /* TODO */
}

static void
seat_update_pointer_cap(pepper_seat_t *seat)
{
    if ((seat->caps & WL_SEAT_CAPABILITY_POINTER) && !seat->pointer.active)
    {
        seat->pointer.active = PEPPER_TRUE;
        pepper_object_init(&seat->pointer.base, PEPPER_OBJECT_POINTER);
        wl_list_init(&seat->pointer.resource_list);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_POINTER_ADD, &seat->pointer);
    }
    else if (!(seat->caps & WL_SEAT_CAPABILITY_POINTER) && seat->pointer.active)
    {
        seat->pointer.active = PEPPER_FALSE;
        pepper_object_fini(&seat->pointer.base);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_POINTER_REMOVE, &seat->pointer);
    }
}

static void
seat_update_keyboard_cap(pepper_seat_t *seat)
{
    if ((seat->caps & WL_SEAT_CAPABILITY_KEYBOARD) && !seat->keyboard.active)
    {
        seat->keyboard.active = PEPPER_TRUE;
        pepper_object_init(&seat->keyboard.base, PEPPER_OBJECT_KEYBOARD);
        wl_list_init(&seat->keyboard.resource_list);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_KEYBOARD_ADD, &seat->keyboard);
    }
    else if (!(seat->caps & WL_SEAT_CAPABILITY_KEYBOARD) && seat->keyboard.active)
    {
        seat->keyboard.active = PEPPER_FALSE;
        pepper_object_fini(&seat->keyboard.base);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_KEYBOARD_REMOVE, &seat->keyboard);
    }
}

static void
seat_update_touch_cap(pepper_seat_t *seat)
{
    if ((seat->caps & WL_SEAT_CAPABILITY_TOUCH) && !seat->touch.active)
    {
        seat->touch.active = PEPPER_TRUE;
        pepper_object_init(&seat->touch.base, PEPPER_OBJECT_TOUCH);
        wl_list_init(&seat->touch.resource_list);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_TOUCH_ADD, &seat->touch);
    }
    else if (!(seat->caps & WL_SEAT_CAPABILITY_TOUCH) && seat->touch.active)
    {
        seat->touch.active = PEPPER_FALSE;
        pepper_object_fini(&seat->touch.base);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_TOUCH_REMOVE, &seat->touch);
    }
}

static void
seat_update_caps(pepper_seat_t *seat)
{
    pepper_list_t  *l;
    uint32_t        caps = 0;

    PEPPER_LIST_FOR_EACH(&seat->input_device_list, l)
    {
        pepper_input_device_entry_t *entry = l->item;
        caps |= entry->device->caps;
    }

    if (caps != seat->caps)
    {
        struct wl_resource *resource;

        seat->caps = caps;

        seat_update_pointer_cap(seat);
        seat_update_keyboard_cap(seat);
        seat_update_touch_cap(seat);

        wl_resource_for_each(resource, &seat->resources)
            wl_seat_send_capabilities(resource, seat->caps);
    }
}

static void
seat_handle_device_event(pepper_event_listener_t *listener, pepper_object_t *object,
                         uint32_t id, void *info, void *data)
{
    pepper_input_device_entry_t *entry = data;

    switch (id)
    {
    case PEPPER_EVENT_OBJECT_DESTROY:
        pepper_seat_remove_input_device(entry->seat, entry->device);
        break;
    case PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE:
        {
            pepper_pointer_t *pointer = pepper_seat_get_pointer(entry->seat);

            pepper_object_emit_event(&pointer->base,
                                     PEPPER_EVENT_POINTER_MOTION,
                                     info);

        }
        break;
    case PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_POINTER_AXIS:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_TOUCH_DOWN:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_TOUCH_UP:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_TOUCH_MOTION:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_TOUCH_FRAME:
        /* TODO: */
        break;
    case PEPPER_EVENT_INPUT_DEVICE_TOUCH_CANCEL:
        /* TODO: */
        break;
    }
}

PEPPER_API void
pepper_seat_add_input_device(pepper_seat_t *seat, pepper_input_device_t *device)
{
    pepper_input_device_entry_t *entry;
    pepper_list_t               *l;

    PEPPER_LIST_FOR_EACH(&seat->input_device_list, l)
    {
        pepper_input_device_entry_t *entry = l->item;
        if (entry->device == device)
            return ;
    }

    entry = pepper_calloc(1, sizeof(pepper_input_device_entry_t));
    if (!entry)
        return;

    entry->seat = seat;
    entry->device = device;

    entry->link.item = entry;
    entry->listener = pepper_object_add_event_listener(&device->base, PEPPER_EVENT_ALL, 0,
                                                       seat_handle_device_event, entry);
    pepper_list_insert(&seat->input_device_list, &entry->link);

    seat_update_caps(seat);
}

PEPPER_API void
pepper_seat_remove_input_device(pepper_seat_t *seat, pepper_input_device_t *device)
{
    pepper_list_t *l;

    PEPPER_LIST_FOR_EACH(&seat->input_device_list, l)
    {
        pepper_input_device_entry_t *entry = l->item;

        if (entry->device == device)
        {
            pepper_list_remove(&entry->link, NULL);
            pepper_event_listener_remove(entry->listener);
            pepper_free(entry);
            seat_update_caps(seat);
            return;
        }
    }
}

PEPPER_API pepper_input_device_t *
pepper_input_device_create(pepper_compositor_t *compositor, uint32_t caps,
                           const pepper_input_device_backend_t *backend, void *data)
{
    pepper_input_device_t  *device;

    device = (pepper_input_device_t *)pepper_object_alloc(PEPPER_OBJECT_INPUT_DEVICE,
                                                            sizeof(pepper_input_device_t));
    if (!device)
    {
        PEPPER_ERROR("Failed to allocate memory\n");
        return NULL;
    }

    device->compositor = compositor;
    device->caps = caps;
    device->backend = backend;
    device->data = data;

    pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
                             device);
    return device;
}

PEPPER_API void
pepper_input_device_destroy(pepper_input_device_t *device)
{
    pepper_object_emit_event(&device->compositor->base,
                             PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,
                             device);
    pepper_object_fini(&device->base);
    pepper_free(device);
}

PEPPER_API const char *
pepper_input_device_get_property(pepper_input_device_t *device, const char *key)
{
    if (!device->backend)
        return NULL;

    return device->backend->get_property(device->data, key);
}

PEPPER_API uint32_t
pepper_input_device_get_caps(pepper_input_device_t *device)
{
    return device->caps;
}
