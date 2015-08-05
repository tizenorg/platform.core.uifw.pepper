#include "pepper-internal.h"

#undef PEPPER_TRACE
#define PEPPER_TRACE(...)

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

    if (!seat->pointer)
        return;

    r = wl_resource_create(client, &wl_pointer_interface,
                           wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->pointer->resources, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &pointer_interface, seat->pointer, unbind_resource);

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

    if (!seat->keyboard)
        return;

    r = wl_resource_create(client, &wl_keyboard_interface,
                           wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->keyboard->resources, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &keyboard_interface, seat->keyboard, unbind_resource);

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

    if (!seat->touch)
        return;

    r = wl_resource_create(client, &wl_touch_interface, wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->touch->resources, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &touch_interface, seat->touch, unbind_resource);

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

    seat->global = wl_global_create(compositor->display, &wl_seat_interface, 4, seat,
                                    bind_seat);
    return seat;
}

PEPPER_API pepper_pointer_t *
pepper_seat_get_pointer(pepper_seat_t *seat)
{
    return seat->pointer;
}

PEPPER_API pepper_keyboard_t *
pepper_seat_get_keyboard(pepper_seat_t *seat)
{
    return seat->keyboard;
}

PEPPER_API pepper_touch_t *
pepper_seat_get_touch(pepper_seat_t *seat)
{
    return seat->touch;
}

PEPPER_API pepper_pointer_device_t *
pepper_pointer_device_create(pepper_compositor_t *compositor)
{
    pepper_pointer_device_t    *device;

    device = (pepper_pointer_device_t *)pepper_object_alloc(PEPPER_OBJECT_POINTER_DEVICE,
                                                            sizeof(pepper_pointer_device_t));
    if (!device)
    {
        PEPPER_ERROR("Failed to allocate memory\n");
        return NULL;
    }

    return device;
}

PEPPER_API void
pepper_pointer_device_destroy(pepper_pointer_device_t *device)
{
    pepper_object_fini((pepper_object_t *)device);
    pepper_free(device);
}

PEPPER_API pepper_keyboard_device_t *
pepper_keyboard_device_create(pepper_compositor_t *compositor)
{
    pepper_keyboard_device_t   *device;

    device = (pepper_keyboard_device_t *)pepper_object_alloc(PEPPER_OBJECT_KEYBOARD_DEVICE,
                                                            sizeof(pepper_keyboard_device_t));
    if (!device)
    {
        PEPPER_ERROR("Failed to allocate memory\n");
        return NULL;
    }

    return device;
}

PEPPER_API void
pepper_keyboard_device_destroy(pepper_keyboard_device_t *device)
{
    pepper_object_fini((pepper_object_t *)device);
    pepper_free(device);
}

PEPPER_API pepper_touch_device_t *
pepper_touch_device_create(pepper_compositor_t *compositor)
{
    pepper_touch_device_t  *device;

    device = (pepper_touch_device_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH_DEVICE,
                                                            sizeof(pepper_touch_device_t));
    if (!device)
    {
        PEPPER_ERROR("Failed to allocate memory\n");
        return NULL;
    }

    return device;
}

PEPPER_API void
pepper_touch_device_destroy(pepper_touch_device_t *device)
{
    pepper_object_fini((pepper_object_t *)device);
    pepper_free(device);
}
