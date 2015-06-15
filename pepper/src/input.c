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
}

static void
handle_seat_set_capabilities(struct wl_listener *listener, void *data)
{
    pepper_seat_t  *seat = pepper_container_of(listener, pepper_seat_t, capabilities_listener);
    uint32_t        caps;

    caps = seat->interface->get_capabilities(data);
    seat_set_capabilities(seat, caps);
}

static void
seat_set_name(pepper_seat_t *seat, const char *name)
{
    struct wl_resource  *resource;

    seat->name = name;
    wl_resource_for_each(resource, &seat->resources)
        wl_seat_send_name(resource, name);
}

static void
handle_seat_set_name(struct wl_listener *listener, void *data)
{
    pepper_seat_t   *seat = pepper_container_of(listener, pepper_seat_t, name_listener);
    const char      *name;

    name = seat->interface->get_name(data);
    seat_set_name(seat, name);
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

void
pepper_seat_update_modifier(pepper_seat_t *seat, pepper_input_event_t *event)
{
    /* TODO */
    seat->modifier = event->value;
}

pepper_bool_t
pepper_compositor_event_handler(pepper_seat_t           *seat,
                                pepper_input_event_t    *event,
                                void                    *data)
{
    /* TODO: */
    /* pepper_compositor_t *compositor = data; */

    switch(event->type)
    {
    case PEPPER_INPUT_EVENT_KEYBOARD_KEY:
        pepper_seat_update_modifier(seat, event);
        break;

    case PEPPER_INPUT_EVENT_POINTER_BUTTON:
        {
            /* FIXME: Send focused client only */
            struct wl_display   *display = pepper_compositor_get_display(seat->compositor);
            uint32_t             serial  = wl_display_next_serial(display);
            struct wl_resource  *pointer;

            wl_resource_for_each(pointer, &seat->pointer->resources)
                wl_pointer_send_button(pointer,
                                       serial,
                                       event->time,
                                       event->index,
                                       event->state);
        }
        break;
    case PEPPER_INPUT_EVENT_POINTER_MOTION:
        {
            /* FIXME: Send focused client only */
            struct wl_resource *pointer;
            wl_resource_for_each(pointer, &seat->pointer->resources)
                wl_pointer_send_motion(pointer,
                                       event->time,
                                       wl_fixed_from_double(event->x),
                                       wl_fixed_from_double(event->y));
        }
        break;
    default:
        PEPPER_TRACE("Unknown pepper input event type [%x]\n", event->type);
        break;
    }

    return PEPPER_TRUE;
}

PEPPER_API pepper_event_hook_t *
pepper_compositor_add_event_hook(pepper_compositor_t      *compositor,
                                 pepper_event_handler_t    handler,
                                 void                     *data)
{
    pepper_event_hook_t *hook;

    if( !handler )
        return NULL;

    hook = pepper_calloc(1, sizeof(pepper_event_hook_t));
    if (!hook)
    {
        PEPPER_ERROR("Failed to allocation\n");
        return NULL;
    }

    hook->handler = handler;
    hook->data    = data;

    wl_list_insert(&compositor->event_hook_chain, &hook->link);

    return hook;
}

PEPPER_API void
pepper_event_hook_destroy(pepper_event_hook_t     *hook)
{
    wl_list_remove(&hook->link);
    pepper_free(hook);
}

PEPPER_API pepper_bool_t
pepper_seat_handle_event(pepper_seat_t *seat, pepper_input_event_t *event)
{
    pepper_compositor_t     *compositor = seat->compositor;
    pepper_event_hook_t     *hook, *tmp;
    pepper_bool_t            ret = PEPPER_FALSE;

    /* Iterate installed hook chain. */
    /* XXX: this code is not thread-safe */
    wl_list_for_each_safe(hook, tmp, &compositor->event_hook_chain, link)
    {
        ret = hook->handler(seat, event, hook->data);

        /* Event consumed, do not delegate to next element */
        if( PEPPER_TRUE == ret)
            break;
    }

    return ret;
}
