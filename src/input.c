#include <dlfcn.h>
#include "pepper_internal.h"

#define PATH_MAX_LEN 128
int
load_input_module(pepper_compositor_t *compositor, const char *input_name)
{
    char path[PATH_MAX_LEN];

    void *module;
    int (*init)(pepper_compositor_t *);

    if (input_name)
    {
        if (input_name[0] != '/')
            snprintf(path, sizeof path, "%s/%s", MODULEDIR, input_name);
        else
            snprintf(path, sizeof path, "%s", input_name);
    }
    else
    {
        snprintf(path, sizeof path, "%s/%s", MODULEDIR, "input_libinput.so");
    }

    module = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
    if (module)
    {
        dlclose(module);
        return 0;
    }

    module = dlopen(path, RTLD_NOW);
    if (!module)
    {
        return -1;
    }

    *(void **)(&init) = dlsym(module, "module_init");
    if (!init)
    {
        dlclose(module);
        return -1;
    }

    init(compositor);

    /* TODO: add input_fd to compositor loop */

    return 0;
}

pepper_seat_t *
pepper_seat_create()
{
    /* TODO: reimplement */

    pepper_seat_t       *seat;
    pepper_pointer_t    *pointer;
    pepper_keyboard_t   *keyboard;
    pepper_touch_t      *touch;

    seat = (pepper_seat_t *)pepper_calloc(1, sizeof(pepper_seat_t));
    if (!seat)
    {
        PEPPER_ERROR("%s Memory allocation failed\n", __FUNCTION__);
        goto err;
    }

    pointer = (pepper_pointer_t *)pepper_calloc(1, sizeof(pepper_pointer_t));
    if (!pointer)
    {
        PEPPER_ERROR("%s Memory allocation failed\n", __FUNCTION__);
        goto err;
    }

    keyboard = (pepper_keyboard_t *)pepper_calloc(1, sizeof(pepper_keyboard_t));
    if (!keyboard)
    {
        PEPPER_ERROR("%s Memory allocation failed\n", __FUNCTION__);
        goto err;
    }

    touch = (pepper_touch_t *)pepper_calloc(1, sizeof(pepper_touch_t));
    if (!touch)
    {
        PEPPER_ERROR("%s Memory allocation failed\n", __FUNCTION__);
        goto err;
    }

    seat->pointer = pointer;
    seat->keyboard = keyboard;
    seat->touch = touch;

    return seat;

err:
    if (seat)
        pepper_free(seat);
    if (pointer)
        pepper_free(pointer);
    if (keyboard)
        pepper_free(keyboard);
    if (touch)
        pepper_free(touch);

    return NULL;
}

static void
pointer_set_cursor(struct wl_client *client, struct wl_resource *resource, uint32_t serial,
                   struct wl_resource *surface_resource, int32_t x, int32_t y)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
    /* TODO: */
}

static void
pointer_release(struct wl_client *client, struct wl_resource *resource)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
    wl_resource_destroy(resource);
}

static void
keyboard_release(struct wl_client *client, struct wl_resource *resource)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
    wl_resource_destroy(resource);
}

static void
touch_release(struct wl_client *client, struct wl_resource *resource)
{
    PEPPER_TRACE("%s\n", __FUNCTION__);
    wl_resource_destroy(resource);
}

static const struct wl_pointer_interface pointer_interface =
{
    pointer_set_cursor,
    pointer_release
};

static const struct wl_keyboard_interface keyboard_interface =
{
    keyboard_release
};

static const struct wl_touch_interface touch_interface =
{
    touch_release
};

static void
seat_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t       *seat = wl_resource_get_user_data(resource);
    pepper_pointer_t    *pointer = seat->pointer;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    pointer->resource = wl_resource_create(client, &wl_pointer_interface,
                                           wl_resource_get_version(resource), id);
    if (!pointer->resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(pointer->resource, &pointer_interface, seat,
                                   NULL /* TODO: destroy() */);
    return;
}

static void
seat_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t       *seat = wl_resource_get_user_data(resource);
    pepper_keyboard_t   *keyboard = seat->keyboard;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    keyboard->resource = wl_resource_create(client, &wl_keyboard_interface,
                                            wl_resource_get_version(resource), id);
    if (!keyboard->resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(keyboard->resource, &keyboard_interface, seat,
                                   NULL /* TODO: destroy() */);
    return;
}

static void
seat_get_touch(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t   *seat = wl_resource_get_user_data(resource);
    pepper_touch_t  *touch = seat->touch;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    touch->resource = wl_resource_create(client, &wl_touch_interface,
                                         wl_resource_get_version(resource), id);
    if (!touch->resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(touch->resource, &touch_interface, seat,
                                   NULL /* TODO: destroy() */);
    return;
}

static const struct wl_seat_interface seat_interface =
{
    seat_get_pointer,
    seat_get_keyboard,
    seat_get_touch
};

void
bind_seat(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    pepper_seat_t           *seat = (pepper_seat_t *)data;
    enum wl_seat_capability caps = 0;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    seat->resource = wl_resource_create(client, &wl_seat_interface,
                                        (version < 4) ? version : 4, id);
    wl_resource_set_implementation(seat->resource, &seat_interface, data,
                                   NULL/* TODO: destroy() */);

    if (seat->pointer)
        caps |= WL_SEAT_CAPABILITY_POINTER;
    if (seat->keyboard)
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    if (seat->touch)
        caps |= WL_SEAT_CAPABILITY_TOUCH;

    wl_seat_send_capabilities(seat->resource, caps);
    if (version >= 2 /* FIXME: WL_SEAT_NAME_SINCE_VERSION */)
        wl_seat_send_name(seat->resource, seat->name);
}
