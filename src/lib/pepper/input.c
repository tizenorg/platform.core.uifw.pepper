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

void
pepper_input_init(pepper_input_t *input, pepper_seat_t *seat)
{
    input->seat = seat;
    input->focus = NULL;

    wl_list_init(&input->resource_list);
    wl_list_init(&input->focus_resource_list);
}

void
pepper_input_fini(pepper_input_t *input)
{
    if (input->focus)
        wl_list_remove(&input->focus_destroy_listener.link);
}

void
pepper_input_bind_resource(pepper_input_t *input,
                           struct wl_client *client, int version, uint32_t id,
                           const struct wl_interface *interface, const void *impl, void *data)
{
    struct wl_resource *resource = wl_resource_create(client, interface, version, id);

    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&input->resource_list, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, impl, data, unbind_resource);
}

void
pepper_input_set_focus(pepper_input_t *input, pepper_view_t *focus)
{
    if (input->focus == focus)
        return;

    PEPPER_ASSERT(!focus || focus->surface != NULL);

    if (input->focus)
    {
        wl_list_insert_list(&input->resource_list, &input->focus_resource_list);
        wl_list_init(&input->focus_resource_list);
        wl_list_remove(&input->focus_destroy_listener.link);
    }

    input->focus = focus;

    if (focus)
    {
        struct wl_resource *resource, *tmp;
        struct wl_client   *client = wl_resource_get_client(focus->surface->resource);

        wl_resource_for_each_safe(resource, tmp, &input->resource_list)
        {
            if (wl_resource_get_client(resource) == client)
            {
                wl_list_remove(wl_resource_get_link(resource));
                wl_list_insert(&input->focus_resource_list, wl_resource_get_link(resource));
            }
        }

        wl_resource_add_destroy_listener(focus->surface->resource, &input->focus_destroy_listener);
    }
}

static const struct wl_seat_interface seat_interface = {
    pepper_pointer_bind_resource,
    pepper_keyboard_bind_resource,
    pepper_touch_bind_resource,
};

static void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    pepper_seat_t *seat = (pepper_seat_t *)data;
    struct wl_resource  *resource;

    resource = wl_resource_create(client, &wl_seat_interface, version/*FIXME*/, id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&seat->resource_list, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, &seat_interface, data, unbind_resource);

    wl_seat_send_capabilities(resource, seat->caps);

    if ((seat->name) && (version >= WL_SEAT_NAME_SINCE_VERSION))
        wl_seat_send_name(resource, seat->name);
}

PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor, const char *seat_name)
{
    pepper_seat_t  *seat;

    PEPPER_CHECK(seat_name, return NULL, "seat name must be given.\n");

    seat = (pepper_seat_t *)pepper_object_alloc(PEPPER_OBJECT_SEAT, sizeof(pepper_seat_t));
    PEPPER_CHECK(seat, return NULL, "Failed to allocate memory in %s\n", __FUNCTION__);

    pepper_list_init(&seat->link);
    wl_list_init(&seat->resource_list);
    pepper_list_init(&seat->input_device_list);

    seat->name = strdup(seat_name);
    PEPPER_CHECK(seat->name, goto error, "strdup() failed.\n");

    seat->global = wl_global_create(compositor->display, &wl_seat_interface, 4, seat, bind_seat);
    PEPPER_CHECK(seat->global, goto error, "wl_global_create() failed.\n");

    seat->compositor = compositor;

    seat->link.item = seat;
    pepper_list_insert(&compositor->seat_list, &seat->link);
    pepper_object_emit_event(&seat->compositor->base, PEPPER_EVENT_COMPOSITOR_SEAT_ADD, seat);

    return seat;

error:
    if (seat)
        pepper_seat_destroy(seat);

    return NULL;
}

PEPPER_API void
pepper_seat_destroy(pepper_seat_t *seat)
{
    struct wl_resource *resource, *tmp;

    if (seat->name)
        free(seat->name);

    if (seat->pointer)
        pepper_pointer_destroy(seat->pointer);

    if (seat->keyboard)
        pepper_keyboard_destroy(seat->keyboard);

    if (seat->touch)
        pepper_touch_destroy(seat->touch);

    if (seat->global)
        wl_global_destroy(seat->global);

    wl_resource_for_each_safe(resource, tmp, &seat->resource_list)
        wl_resource_destroy(resource);
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

PEPPER_API const char *
pepper_seat_get_name(pepper_seat_t *seat)
{
    return seat->name;
}

static void
seat_update_pointer_cap(pepper_seat_t *seat)
{
    if ((seat->caps & WL_SEAT_CAPABILITY_POINTER) && !seat->pointer)
    {
        seat->pointer = pepper_pointer_create(seat);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_POINTER_ADD, &seat->pointer);
    }
    else if (!(seat->caps & WL_SEAT_CAPABILITY_POINTER) && seat->pointer)
    {
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_POINTER_REMOVE, &seat->pointer);
        pepper_pointer_destroy(seat->pointer);
        seat->pointer = NULL;
    }
}

static void
seat_update_keyboard_cap(pepper_seat_t *seat)
{
    if ((seat->caps & WL_SEAT_CAPABILITY_KEYBOARD) && !seat->keyboard)
    {
        seat->keyboard = pepper_keyboard_create(seat);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_KEYBOARD_ADD, &seat->keyboard);
    }
    else if (!(seat->caps & WL_SEAT_CAPABILITY_KEYBOARD) && seat->keyboard)
    {
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_KEYBOARD_REMOVE, &seat->keyboard);
        pepper_keyboard_destroy(seat->keyboard);
        seat->keyboard = NULL;
    }
}

static void
seat_update_touch_cap(pepper_seat_t *seat)
{
    if ((seat->caps & WL_SEAT_CAPABILITY_TOUCH) && !seat->touch)
    {
        seat->touch = pepper_touch_create(seat);
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_TOUCH_ADD, &seat->touch);
    }
    else if (!(seat->caps & WL_SEAT_CAPABILITY_TOUCH) && seat->touch)
    {
        pepper_object_emit_event(&seat->base, PEPPER_EVENT_SEAT_TOUCH_REMOVE, &seat->touch);
        pepper_touch_destroy(seat->touch);
        seat->touch = NULL;
    }
}

static void
seat_update_caps(pepper_seat_t *seat)
{
    uint32_t                        caps = 0;
    pepper_input_device_entry_t    *entry;

    pepper_list_for_each(entry, &seat->input_device_list, link)
        caps |= entry->device->caps;

    if (caps != seat->caps)
    {
        struct wl_resource *resource;

        seat->caps = caps;

        seat_update_pointer_cap(seat);
        seat_update_keyboard_cap(seat);
        seat_update_touch_cap(seat);

        wl_resource_for_each(resource, &seat->resource_list)
            wl_seat_send_capabilities(resource, seat->caps);
    }
}

static void
seat_handle_device_event(pepper_event_listener_t *listener, pepper_object_t *object,
                         uint32_t id, void *info, void *data)
{
    pepper_input_device_entry_t *entry = data;
    pepper_seat_t               *seat = entry->seat;

    switch (id)
    {
    case PEPPER_EVENT_OBJECT_DESTROY:
        pepper_seat_remove_input_device(seat, entry->device);
        break;
    case PEPPER_EVENT_POINTER_MOTION_ABSOLUTE:
    case PEPPER_EVENT_POINTER_MOTION:
    case PEPPER_EVENT_POINTER_BUTTON:
    case PEPPER_EVENT_POINTER_AXIS:
        pepper_object_emit_event(&seat->pointer->base, id, info);
        break;
    case PEPPER_EVENT_KEYBOARD_KEY:
        pepper_object_emit_event(&seat->keyboard->base, id, info);
        break;
    case PEPPER_EVENT_TOUCH_DOWN:
    case PEPPER_EVENT_TOUCH_UP:
    case PEPPER_EVENT_TOUCH_MOTION:
    case PEPPER_EVENT_TOUCH_FRAME:
    case PEPPER_EVENT_TOUCH_CANCEL:
        pepper_object_emit_event(&seat->touch->base, id, info);
        break;
    }
}

PEPPER_API void
pepper_seat_add_input_device(pepper_seat_t *seat, pepper_input_device_t *device)
{
    pepper_input_device_entry_t *entry;

    pepper_list_for_each(entry, &seat->input_device_list, link)
    {
        if (entry->device == device)
            return;
    }

    entry = calloc(1, sizeof(pepper_input_device_entry_t));
    PEPPER_CHECK(entry, return, "calloc() failed.\n");

    entry->seat = seat;
    entry->device = device;
    pepper_list_insert(&seat->input_device_list, &entry->link);

    seat_update_caps(seat);

    entry->listener = pepper_object_add_event_listener(&device->base, PEPPER_EVENT_ALL, 0,
                                                       seat_handle_device_event, entry);
}

PEPPER_API void
pepper_seat_remove_input_device(pepper_seat_t *seat, pepper_input_device_t *device)
{
    pepper_input_device_entry_t *entry;

    pepper_list_for_each(entry, &seat->input_device_list, link)
    {
        if (entry->device == device)
        {
            pepper_list_remove(&entry->link);
            pepper_event_listener_remove(entry->listener);
            free(entry);
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
    PEPPER_CHECK(device, return NULL, "pepper_object_alloc() failed.\n");

    device->compositor = compositor;
    device->caps = caps;
    device->backend = backend;
    device->data = data;
    device->link.item = device;

    pepper_list_insert(&compositor->input_device_list, &device->link);
    pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
                             device);
    return device;
}

PEPPER_API void
pepper_input_device_destroy(pepper_input_device_t *device)
{
    pepper_object_emit_event(&device->compositor->base,
                             PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE, device);
    pepper_list_remove(&device->link);
    pepper_object_fini(&device->base);
    free(device);
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
