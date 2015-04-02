#include "pepper-internal.h"

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
    return;
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
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    struct wl_resource  *r;

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

    return;
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
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    struct wl_resource  *r;

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
    wl_resource_set_implementation(r, &keyboard_interface, seat, unbind_resource);

    /* TODO */

    return;
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
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    struct wl_resource  *r;

    if (!seat->touch)
        return;

    r = wl_resource_create(client, &wl_touch_interface, wl_resource_get_version(resource), id);
    if (!r)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->touch->resources, wl_resource_get_link(r));
    wl_resource_set_implementation(r, &touch_interface, seat, unbind_resource);

    /* TODO */

    return;
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
}

static pepper_pointer_t *
pointer_create(pepper_seat_t *seat)
{
    pepper_pointer_t *pointer;

    pointer = (pepper_pointer_t *)pepper_calloc(1, sizeof(pepper_pointer_t));
    if (!pointer)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    pointer->seat = seat;
    wl_list_init(&pointer->resources);

    return pointer;
}

static void
pointer_destroy(pepper_seat_t *seat)
{
    struct wl_resource  *resource;
    struct wl_list      *resource_list = &seat->pointer->resources;

    wl_resource_for_each(resource, resource_list)
        wl_resource_destroy(resource);
    pepper_free(seat->pointer);

    return;
}

static pepper_keyboard_t *
keyboard_create(pepper_seat_t *seat)
{
    pepper_keyboard_t   *keyboard;

    keyboard = (pepper_keyboard_t *)pepper_calloc(1, sizeof(pepper_keyboard_t));
    if (!keyboard)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    keyboard->seat = seat;
    wl_list_init(&keyboard->resources);

    return keyboard;
}

static void
keyboard_destroy(pepper_seat_t *seat)
{
    struct wl_resource  *resource;
    struct wl_list      *resource_list = &seat->keyboard->resources;

    wl_resource_for_each(resource, resource_list)
        wl_resource_destroy(resource);
    pepper_free(seat->keyboard);

    return;
}

static pepper_touch_t *
touch_create(pepper_seat_t *seat)
{
    pepper_touch_t *touch;

    touch = (pepper_touch_t *)pepper_calloc(1, sizeof(pepper_touch_t));
    if (!touch)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    touch->seat = seat;
    wl_list_init(&touch->resources);

    return touch;
}

static void
touch_destroy(pepper_seat_t *seat)
{
    struct wl_resource  *resource;
    struct wl_list      *resource_list = &seat->touch->resources;

    wl_resource_for_each(resource, resource_list)
        wl_resource_destroy(resource);
    pepper_free(seat->touch);

    return;
}

static void
seat_set_capabilities(pepper_seat_t *seat, uint32_t caps)
{
    struct wl_resource  *resource;

    seat->caps = caps;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && (!seat->pointer))
    {
        seat->pointer = pointer_create(seat);
        if (!seat->pointer)
        {
            PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
            return;
        }
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && (seat->pointer))
    {
        pointer_destroy(seat);
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && (!seat->keyboard))
    {
        seat->keyboard = keyboard_create(seat);
        if (!seat->keyboard)
        {
            PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
            return;
        }
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && (seat->keyboard))
    {
        keyboard_destroy(seat);
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && (!seat->touch))
    {
        seat->touch = touch_create(seat);
        if (!seat->touch)
        {
            PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
            return;
        }
    }
    else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && (seat->touch))
    {
        touch_destroy(seat);
    }

    wl_resource_for_each(resource, &seat->resources)
        wl_seat_send_capabilities(resource, caps);

    return;
}

static void
handle_seat_set_capabilities(struct wl_listener *listener, void *data)
{
    pepper_seat_t  *seat = wl_container_of(listener, seat, capabilities_listener);
    uint32_t        caps;

    caps = seat->interface->get_capabilities(data);
    seat_set_capabilities(seat, caps);

    return;
}

static void
seat_set_name(pepper_seat_t *seat, const char *name)
{
    struct wl_resource  *resource;

    seat->name = name;
    wl_resource_for_each(resource, &seat->resources)
        wl_seat_send_name(resource, name);

    return;
}

static void
handle_seat_set_name(struct wl_listener *listener, void *data)
{
    pepper_seat_t   *seat = wl_container_of(listener, seat, name_listener);
    const char      *name;

    name = seat->interface->get_name(data);
    seat_set_name(seat, name);

    return;
}

PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor,
                           const pepper_seat_interface_t *interface,
                           void *data)
{
    pepper_seat_t *seat;

    seat = (pepper_seat_t *)pepper_calloc(1, sizeof(pepper_seat_t));
    if (!seat)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    seat->compositor = compositor;
    seat->interface = (pepper_seat_interface_t *)interface;
    seat->data = data;

    wl_list_init(&seat->resources);
    wl_list_init(&seat->link);
    wl_list_insert(&compositor->seat_list, &seat->link);

    seat->capabilities_listener.notify = handle_seat_set_capabilities;
    seat->interface->add_capabilities_listener(seat->data, &seat->capabilities_listener);
    seat->name_listener.notify = handle_seat_set_name;
    seat->interface->add_name_listener(seat->data, &seat->name_listener);

    seat->global = wl_global_create(compositor->display, &wl_seat_interface, 4, seat,
                                    bind_seat);

    return seat;
}

PEPPER_API pepper_bool_t
pepper_seat_handle_event(pepper_seat_t *seat, pepper_input_event_t *event)
{
    /* TODO */
    return PEPPER_TRUE;
}
