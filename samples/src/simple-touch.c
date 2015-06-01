/*
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2011 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

struct seat {
    struct touch *touch;
    struct wl_seat *seat;
    struct wl_pointer *wl_pointer;
    struct wl_keyboard *wl_keyboard;
    struct wl_touch *wl_touch;
};

struct touch {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shell *shell;
    struct wl_shm *shm;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    struct wl_buffer *buffer;
    int has_argb;
    int width, height;
    int x, y;
    void *data;
};

static void
create_shm_buffer(struct touch *touch)
{
    struct wl_shm_pool *pool;
    int fd, size, stride;

    stride = touch->width * 4;
    size = stride * touch->height;

    fd = pepper_create_anonymous_file(size);
    if (fd < 0) {
        fprintf(stderr, "creating a buffer file for %d B failed: %m\n",
                size);
        exit(1);
    }

    touch->data =
        mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (touch->data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        exit(1);
    }

    pool = wl_shm_create_pool(touch->shm, fd, size);
    touch->buffer =
        wl_shm_pool_create_buffer(pool, 0,
                                  touch->width, touch->height, stride,
                                  WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    close(fd);
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    struct touch *touch = data;

    if (format == WL_SHM_FORMAT_ARGB8888)
        touch->has_argb = 1;
}

struct wl_shm_listener shm_listener = {
    shm_format
};


static void
touch_paint(struct touch *touch, int32_t x, int32_t y, int32_t id)
{
    uint32_t *p, c;
    static const uint32_t colors[] = {
        0xffff0000,
        0xff0000ff,
        0xff00ff00,
        0xff000000,
    };

    if (id < (int32_t) ARRAY_LENGTH(colors))
        c = colors[id];
    else
        c = 0xffffffff;

    if (x < 2 || x >= touch->width - 2 ||
        y < 2 || y >= touch->height - 2)
        return;

    p = (uint32_t *) touch->data + (x - 2) + (y - 2) * touch->width;
    p[2] = c;
    p += touch->width;
    p[1] = c;
    p[2] = c;
    p[3] = c;
    p += touch->width;
    p[0] = c;
    p[1] = c;
    p[2] = c;
    p[3] = c;
    p[4] = c;
    p += touch->width;
    p[1] = c;
    p[2] = c;
    p[3] = c;
    p += touch->width;
    p[2] = c;

    wl_surface_attach(touch->surface, touch->buffer, 0, 0);
    wl_surface_damage(touch->surface, x - 2, y - 2, 5, 5);
    /* todo: We could queue up more damage before committing, if there
     * are more input events to handle.
     */
    wl_surface_commit(touch->surface);
}

static uint32_t pointer_state;
static wl_fixed_t pointer_x;
static wl_fixed_t pointer_y;

static void
pointer_handle_enter(void *data,
                     struct wl_pointer *wl_pointer,
                     uint32_t serial,
                     struct wl_surface *surface,
                     wl_fixed_t surface_x,
                     wl_fixed_t surface_y)
{
    pointer_x = surface_x;
    pointer_y = surface_y;
}

static void
pointer_handle_leave(void *data,
                     struct wl_pointer *wl_pointer,
                     uint32_t serial,
                     struct wl_surface *surface)
{
}

static void
pointer_handle_motion(void *data,
                      struct wl_pointer *wl_pointer,
                      uint32_t time,
                      wl_fixed_t surface_x,
                      wl_fixed_t surface_y)
{
    struct touch *touch = data;
    float x;
    float y;

    pointer_x = surface_x;
    pointer_y = surface_y;

    x = wl_fixed_to_double(pointer_x);
    y = wl_fixed_to_double(pointer_y);

    if (pointer_state == WL_POINTER_BUTTON_STATE_PRESSED)
        touch_paint(touch, x, y, 2);
}

static void
pointer_handle_button(void *data,
                      struct wl_pointer *wl_pointer,
                      uint32_t serial,
                      uint32_t time,
                      uint32_t button,
                      uint32_t state)
{
    struct touch *touch = data;
    float x = wl_fixed_to_double(pointer_x);
    float y = wl_fixed_to_double(pointer_y);

    pointer_state = state;

    touch_paint(touch, x, y, state);
}

static void
pointer_handle_axis(void *data,
                    struct wl_pointer *wl_pointer,
                    uint32_t time,
                    uint32_t axis,
                    wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
};

static void
keyboard_handle_keymap(void *data,
                       struct wl_keyboard *wl_keyboard,
                       uint32_t format,
                       int32_t fd,
                       uint32_t size)
{
}

static void
keyboard_handle_enter(void *data,
                      struct wl_keyboard *wl_keyboard,
                      uint32_t serial,
                      struct wl_surface *surface,
                      struct wl_array *keys)
{
}

static void
keyboard_handle_leave(void *data,
                      struct wl_keyboard *wl_keyboard,
                      uint32_t serial,
                      struct wl_surface *surface)
{
}

static void
keyboard_handle_key(void *data,
                    struct wl_keyboard *wl_keyboard,
                    uint32_t serial,
                    uint32_t time,
                    uint32_t key,
                    uint32_t state)
{
    struct touch   *touch = data;

    touch_paint(touch, touch->x, touch->y, 3);

    touch->x += 4;
    if (touch->x >= (touch->width - 2)) {
        touch->x = 2;
        touch->y += 4;
        if (touch->y >= (touch->height - 2))
        {
            touch->y = 2;
        }
    }
}

static void
keyboard_handle_modifiers(void *data,
                          struct wl_keyboard *wl_keyboard,
                          uint32_t serial,
                          uint32_t mods_depressed,
                          uint32_t mods_latched,
                          uint32_t mods_locked,
                          uint32_t group)
{
}

static void
keyboard_handle_repeat_info(void *data,
                            struct wl_keyboard *wl_keyboard,
                            int32_t rate,
                            int32_t delay)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
    keyboard_handle_repeat_info,
};

static void
touch_handle_down(void *data, struct wl_touch *wl_touch,
                  uint32_t serial, uint32_t time, struct wl_surface *surface,
                  int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    struct touch *touch = data;
    float x = wl_fixed_to_double(x_w);
    float y = wl_fixed_to_double(y_w);

    touch_paint(touch, x, y, id);
}

static void
touch_handle_up(void *data, struct wl_touch *wl_touch,
                uint32_t serial, uint32_t time, int32_t id)
{
}

static void
touch_handle_motion(void *data, struct wl_touch *wl_touch,
                    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    struct touch *touch = data;
    float x = wl_fixed_to_double(x_w);
    float y = wl_fixed_to_double(y_w);

    touch_paint(touch, x, y, id);
}

static void
touch_handle_frame(void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel(void *data, struct wl_touch *wl_touch)
{
}

static const struct wl_touch_listener touch_listener = {
    touch_handle_down,
    touch_handle_up,
    touch_handle_motion,
    touch_handle_frame,
    touch_handle_cancel,
};

static void
seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
                         enum wl_seat_capability caps)
{
    struct seat *seat = data;
    struct touch *touch = seat->touch;

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !seat->wl_pointer) {
        seat->wl_pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_set_user_data(seat->wl_pointer, touch);
        wl_pointer_add_listener(seat->wl_pointer, &pointer_listener, touch);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && seat->wl_pointer) {
        wl_pointer_destroy(seat->wl_pointer);
        seat->wl_pointer = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !seat->wl_keyboard) {
        seat->wl_keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_set_user_data(seat->wl_keyboard, touch);
        wl_keyboard_add_listener(seat->wl_keyboard, &keyboard_listener, touch);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && seat->wl_keyboard) {
        wl_keyboard_destroy(seat->wl_keyboard);
        seat->wl_keyboard = NULL;
    }

    if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !seat->wl_touch) {
        seat->wl_touch = wl_seat_get_touch(wl_seat);
        wl_touch_set_user_data(seat->wl_touch, touch);
        wl_touch_add_listener(seat->wl_touch, &touch_listener, touch);
    } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && seat->wl_touch) {
        wl_touch_destroy(seat->wl_touch);
        seat->wl_touch = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

static void
add_seat(struct touch *touch, uint32_t name, uint32_t version)
{
    struct seat *seat;

    seat = malloc(sizeof *seat);
    assert(seat);

    seat->touch = touch;
    seat->wl_pointer = NULL;
    seat->wl_keyboard = NULL;
    seat->wl_touch = NULL;
    seat->seat = wl_registry_bind(touch->registry, name,
                                  &wl_seat_interface, 1);
    wl_seat_add_listener(seat->seat, &seat_listener, seat);
}

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
            uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done
};

static void
handle_global(void *data, struct wl_registry *registry,
              uint32_t name, const char *interface, uint32_t version)
{
    struct touch *touch = data;

    if (strcmp(interface, "wl_compositor") == 0) {
        touch->compositor =
            wl_registry_bind(registry, name,
                             &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        touch->shell =
            wl_registry_bind(registry, name,
                             &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        touch->shm = wl_registry_bind(registry, name,
                                      &wl_shm_interface, 1);
        wl_shm_add_listener(touch->shm, &shm_listener, touch);
    } else if (strcmp(interface, "wl_seat") == 0) {
        add_seat(touch, name, version);
    }
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    handle_global,
    handle_global_remove
};

static struct touch *
touch_create(int width, int height, const char *display_name)
{
    struct touch *touch;

    touch = malloc(sizeof *touch);
    if (touch == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    touch->display = wl_display_connect(display_name);
    assert(touch->display);

    touch->has_argb = 0;
    touch->registry = wl_display_get_registry(touch->display);
    wl_registry_add_listener(touch->registry, &registry_listener, touch);
    wl_display_dispatch(touch->display);
    wl_display_roundtrip(touch->display);

    if (!touch->has_argb) {
        fprintf(stderr, "WL_SHM_FORMAT_ARGB32 not available\n");
        exit(1);
    }

    touch->width = width;
    touch->height = height;
    touch->x = 2;
    touch->y = 2;
    touch->surface = wl_compositor_create_surface(touch->compositor);
    touch->shell_surface = wl_shell_get_shell_surface(touch->shell,
                                                      touch->surface);
    create_shm_buffer(touch);

    if (touch->shell_surface) {
        wl_shell_surface_add_listener(touch->shell_surface,
                                      &shell_surface_listener, touch);
        wl_shell_surface_set_toplevel(touch->shell_surface);
    }

    wl_surface_set_user_data(touch->surface, touch);
    wl_shell_surface_set_title(touch->shell_surface, "simple-touch");

    memset(touch->data, 64, width * height * 4);
    wl_surface_attach(touch->surface, touch->buffer, 0, 0);
    wl_surface_damage(touch->surface, 0, 0, width, height);
    wl_surface_commit(touch->surface);

    return touch;
}

int
main(int argc, char **argv)
{
    struct touch *touch;
    int ret = 0;
    const char *display_name;

    if (argc == 1)
    {
        display_name = NULL;
    }
    else if (argc == 2)
    {
        display_name = argv[1];
    }
    else
    {
        printf("usage: simple-touch DISPLAY_NAME");
        return 1;
    }

    if (argc != 2)
    {
        printf("usage: simple-touch DISPLAY_NAME");
        return 1;
    }

    touch = touch_create(600, 500, display_name);

    while (ret != -1)
        ret = wl_display_dispatch(touch->display);

    return 0;
}
