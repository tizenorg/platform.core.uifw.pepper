#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "pepper-internal.h"

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_keyboard_interface keyboard_impl =
{
    keyboard_release,
};

static void
clear_keymap(pepper_keyboard_t *keyboard)
{
    if (keyboard->keymap)
    {
        xkb_keymap_unref(keyboard->keymap);
        keyboard->keymap = NULL;
    }

    if (keyboard->keymap_fd >= 0)
    {
        close(keyboard->keymap_fd);
        keyboard->keymap_fd = -1;
        keyboard->keymap_len = -1;
    }

    if (keyboard->pending_keymap)
    {
        xkb_keymap_unref(keyboard->pending_keymap);
        keyboard->pending_keymap = NULL;
    }
}

static void
update_keymap(pepper_keyboard_t *keyboard)
{
    struct wl_resource             *resource;
    char                           *keymap_str = NULL;
    char                           *keymap_map = NULL;

    if (keyboard->keymap)
        xkb_keymap_unref(keyboard->keymap);

    if (keyboard->keymap_fd)
        close(keyboard->keymap_fd);

    keyboard->keymap = xkb_keymap_ref(keyboard->pending_keymap);
    xkb_keymap_unref(keyboard->pending_keymap);
    keyboard->pending_keymap = NULL;

    keymap_str = xkb_keymap_get_as_string(keyboard->keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    PEPPER_CHECK(keymap_str, goto error, "failed to get keymap string\n");

    keyboard->keymap_len = strlen(keymap_str) + 1;
    keyboard->keymap_fd = pepper_create_anonymous_file(keyboard->keymap_len);
    PEPPER_CHECK(keyboard->keymap_fd, goto error, "failed to create keymap file\n");

    keymap_map = mmap(NULL, keyboard->keymap_len, PROT_READ | PROT_WRITE, MAP_SHARED,
                      keyboard->keymap_fd, 0);
    PEPPER_CHECK(keymap_map, goto error, "failed to mmap for keymap\n");

    strcpy(keymap_map, keymap_str);

    wl_resource_for_each(resource, &keyboard->resource_list)
        wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                                keyboard->keymap_fd, keyboard->keymap_len);
    goto done;

error:
    clear_keymap(keyboard);

done:
    if (keymap_map)
        munmap(keymap_map, keyboard->keymap_len);

    if (keymap_str)
        free(keymap_str);
}

void
pepper_keyboard_handle_event(pepper_keyboard_t *keyboard, uint32_t id, pepper_input_event_t *event)
{
    uint32_t       *keys = keyboard->keys.data;
    unsigned int    num_keys = keyboard->keys.size / sizeof(uint32_t);
    unsigned int    i;

    if (id != PEPPER_EVENT_KEYBOARD_KEY)
        return;

    /* Update the array of pressed keys. */
    for (i = 0; i < num_keys; i++)
    {
        if (keys[i] == event->key)
        {
            keys[i] = keys[--num_keys];
            break;
        }
    }

    keyboard->keys.size = num_keys * sizeof(uint32_t);

    if (event->state == PEPPER_KEY_STATE_PRESSED)
        *(uint32_t *)wl_array_add(&keyboard->keys, sizeof(uint32_t)) = event->key;

    if (keyboard->grab)
        keyboard->grab->key(keyboard, keyboard->data, event->time, event->key, event->state);

    if (keyboard->pending_keymap && (keyboard->keys.size == 0))
        update_keymap(keyboard);

    pepper_object_emit_event(&keyboard->base, id, event);
}

static void
keyboard_handle_focus_destroy(struct wl_listener *listener, void *data)
{
    pepper_keyboard_t *keyboard = pepper_container_of(listener, keyboard, focus_destroy_listener);
    pepper_keyboard_set_focus(keyboard, NULL);

    if (keyboard->grab)
        keyboard->grab->cancel(keyboard, keyboard->data);
}

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat)
{
    pepper_keyboard_t *keyboard =
        (pepper_keyboard_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_keyboard_t));

    PEPPER_CHECK(keyboard, return NULL, "pepper_object_alloc() failed.\n");

    keyboard->seat = seat;
    wl_list_init(&keyboard->resource_list);
    keyboard->focus_destroy_listener.notify = keyboard_handle_focus_destroy;

    wl_array_init(&keyboard->keys);

    return keyboard;
}

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard)
{
    clear_keymap(keyboard);

    if (keyboard->grab)
        keyboard->grab->cancel(keyboard, keyboard->data);

    if (keyboard->focus)
        wl_list_remove(&keyboard->focus_destroy_listener.link);

    wl_array_release(&keyboard->keys);
    free(keyboard);
}

static void
unbind_resource(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

void
pepper_keyboard_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t      *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);
    pepper_keyboard_t  *keyboard = seat->keyboard;
    struct wl_resource *res;

    if (!keyboard)
        return;

    res = wl_resource_create(client, &wl_keyboard_interface, wl_resource_get_version(resource), id);
    if (!res)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&keyboard->resource_list, wl_resource_get_link(res));
    wl_resource_set_implementation(res, &keyboard_impl, keyboard, unbind_resource);

    /* TODO: send repeat info */

    if (keyboard->keymap)
    {
        wl_keyboard_send_keymap(res, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                                keyboard->keymap_fd, keyboard->keymap_len);
    }
    else
    {
        int fd = open("/dev/null", O_RDONLY);
        wl_keyboard_send_keymap(res, WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP, fd, 0);
        close(fd);
    }

    if (!keyboard->focus)
        return;

    if (wl_resource_get_client(keyboard->focus->surface->resource) == client)
    {
        wl_keyboard_send_enter(res, keyboard->focus_serial,
                              keyboard->focus->surface->resource, &keyboard->keys);
    }
}

PEPPER_API struct wl_list *
pepper_keyboard_get_resource_list(pepper_keyboard_t *keyboard)
{
    return &keyboard->resource_list;
}

PEPPER_API pepper_compositor_t *
pepper_keyboard_get_compositor(pepper_keyboard_t *keyboard)
{
    return keyboard->seat->compositor;
}

PEPPER_API pepper_seat_t *
pepper_keyboard_get_seat(pepper_keyboard_t *keyboard)
{
    return keyboard->seat;
}

PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *focus)
{
    if (keyboard->focus == focus)
        return;

    if (keyboard->focus)
    {
        pepper_keyboard_send_leave(keyboard);
        wl_list_remove(&keyboard->focus_destroy_listener.link);
        pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_FOCUS_LEAVE, keyboard->focus);
        pepper_object_emit_event(&keyboard->focus->base, PEPPER_EVENT_FOCUS_LEAVE, keyboard);
    }

    keyboard->focus = focus;

    if (focus)
    {
        pepper_keyboard_send_enter(keyboard);
        wl_resource_add_destroy_listener(focus->surface->resource, &keyboard->focus_destroy_listener);
        keyboard->focus_serial = wl_display_next_serial(keyboard->seat->compositor->display);
        pepper_object_emit_event(&keyboard->base, PEPPER_EVENT_FOCUS_ENTER, focus);
        pepper_object_emit_event(&focus->base, PEPPER_EVENT_FOCUS_ENTER, keyboard);
    }
}

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard)
{
    return keyboard->focus;
}

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard)
{
    struct wl_resource *resource;
    struct wl_client   *client;
    uint32_t            serial;

    if (!keyboard->focus)
        return;

    client = wl_resource_get_client(keyboard->focus->surface->resource);
    serial = wl_display_next_serial(keyboard->seat->compositor->display);

    wl_resource_for_each(resource, &keyboard->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_keyboard_send_leave(resource, serial, keyboard->focus->surface->resource);
    }
}

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard)
{
    struct wl_resource *resource;
    struct wl_client   *client;
    uint32_t            serial;

    if (!keyboard->focus)
        return;

    client = wl_resource_get_client(keyboard->focus->surface->resource);
    serial = wl_display_next_serial(keyboard->seat->compositor->display);

    wl_resource_for_each(resource, &keyboard->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
        {
            wl_keyboard_send_enter(resource, serial,
                                   keyboard->focus->surface->resource, &keyboard->keys);
        }
    }
}

PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, uint32_t time, uint32_t key, uint32_t state)
{
    struct wl_resource *resource;
    struct wl_client   *client;
    uint32_t            serial;

    if (!keyboard->focus)
        return;

    client = wl_resource_get_client(keyboard->focus->surface->resource);
    serial = wl_display_next_serial(keyboard->seat->compositor->display);

    wl_resource_for_each(resource, &keyboard->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_keyboard_send_key(resource, serial, time, key, state);
    }
}

PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard, uint32_t depressed, uint32_t latched,
                               uint32_t locked, uint32_t group)
{
    struct wl_resource *resource;
    struct wl_client   *client;
    uint32_t            serial;

    if (!keyboard->focus)
        return;

    client = wl_resource_get_client(keyboard->focus->surface->resource);
    serial = wl_display_next_serial(keyboard->seat->compositor->display);

    wl_resource_for_each(resource, &keyboard->resource_list)
    {
        if (wl_resource_get_client(resource) == client)
            wl_keyboard_send_modifiers(resource, serial, depressed, latched, locked, group);
    }
}

PEPPER_API void
pepper_keyboard_set_grab(pepper_keyboard_t *keyboard, const pepper_keyboard_grab_t *grab, void *data)
{
    keyboard->grab = grab;
    keyboard->data = data;
}

PEPPER_API const pepper_keyboard_grab_t *
pepper_keyboard_get_grab(pepper_keyboard_t *keyboard)
{
    return keyboard->grab;
}

PEPPER_API void *
pepper_keyboard_get_grab_data(pepper_keyboard_t *keyboard)
{
    return keyboard->data;
}

PEPPER_API void
pepper_keyboard_set_keymap(pepper_keyboard_t *keyboard, struct xkb_keymap *keymap)
{
    xkb_keymap_unref(keyboard->pending_keymap);
    keyboard->pending_keymap = xkb_keymap_ref(keymap);
    if (keyboard->keys.size == 0)
        update_keymap(keyboard);
}
