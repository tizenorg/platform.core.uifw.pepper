/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
* 
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

#include <wayland-client.h>
#include <pepper-utils.h>

#include <sys/types.h>

struct display {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct wl_shell *shell;
    uint32_t formats;
};

struct buffer {
    struct wl_buffer *buffer;
    void *shm_data;
    int busy;
};

struct window {
    struct display *display;
    int width, height;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    struct buffer buffers[2];
    struct buffer *prev_buffer;
    struct wl_callback *callback;
};

static int running = 1;

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
    struct buffer *mybuf = data;

    mybuf->busy = 0;
}

static const struct wl_buffer_listener buffer_listener = {
    buffer_release
};

static int
create_shm_buffer(struct display *display, struct buffer *buffer,
                  int width, int height, uint32_t format)
{
    struct wl_shm_pool *pool;
    int fd, size, stride;
    void *data;

    stride = width * 4;
    size = stride * height;

    fd = pepper_create_anonymous_file(size);
    if (fd < 0) {
        fprintf(stderr, "creating a buffer file for %d B failed: %m\n",
                size);
        return -1;
    }

    data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        return -1;
    }

    pool = wl_shm_create_pool(display->shm, fd, size);
    buffer->buffer = wl_shm_pool_create_buffer(pool, 0,
                                               width, height,
                                               stride, format);
    wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
    wl_shm_pool_destroy(pool);
    close(fd);

    buffer->shm_data = data;

    return 0;
}

static void
shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void
shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges,
                        int32_t w, int32_t h)
{
}

static void
shell_surface_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener =
{
    shell_surface_ping,
    shell_surface_configure,
    shell_surface_popup_done,
};

static struct window *
create_window(struct display *display, int width, int height)
{
    struct window *window;

    window = calloc(1, sizeof *window);
    if (!window)
        return NULL;

    window->callback = NULL;
    window->display = display;
    window->width = width;
    window->height = height;
    window->surface = wl_compositor_create_surface(display->compositor);

    if (display->shell) {
        window->shell_surface =
            wl_shell_get_shell_surface(display->shell,
                                       window->surface);

        wl_shell_surface_add_listener(window->shell_surface,
                                      &shell_surface_listener, window);
        wl_shell_surface_set_title(window->shell_surface, "simple-shm");
        wl_shell_surface_set_toplevel(window->shell_surface);
    } else {
        assert(0);
    }

    return window;
}

static void
destroy_window(struct window *window)
{
    if (window->callback)
        wl_callback_destroy(window->callback);

    if (window->buffers[0].buffer)
        wl_buffer_destroy(window->buffers[0].buffer);
    if (window->buffers[1].buffer)
        wl_buffer_destroy(window->buffers[1].buffer);

    wl_surface_destroy(window->surface);
    free(window);
}

static struct buffer *
window_next_buffer(struct window *window)
{
    struct buffer *buffer;
    int ret = 0;

    if (!window->buffers[0].busy)
        buffer = &window->buffers[0];
    else if (!window->buffers[1].busy)
        buffer = &window->buffers[1];
    else
        return NULL;

    if (!buffer->buffer) {
        ret = create_shm_buffer(window->display, buffer,
                                window->width, window->height,
                                WL_SHM_FORMAT_XRGB8888);

        if (ret < 0)
            return NULL;

        /* paint the padding */
        memset(buffer->shm_data, 0xff,
               window->width * window->height * 4);
    }

    return buffer;
}

static void
paint_pixels(void *image, int padding, int width, int height, uint32_t time)
{
    const int halfh = padding + (height - padding * 2) / 2;
    const int halfw = padding + (width  - padding * 2) / 2;
    int ir, or;
    uint32_t *pixel = image;
    int y;

    /* squared radii thresholds */
    or = (halfw < halfh ? halfw : halfh) - 8;
    ir = or - 32;
    or *= or;
    ir *= ir;

    pixel += padding * width;
    for (y = padding; y < height - padding; y++) {
        int x;
        int y2 = (y - halfh) * (y - halfh);

        pixel += padding;
        for (x = padding; x < width - padding; x++) {
            uint32_t v;

            /* squared distance from center */
            int r2 = (x - halfw) * (x - halfw) + y2;

            if (r2 < ir)
                v = (r2 / 32 + time / 64) * 0x0080401;
            else if (r2 < or)
                v = (y + time / 32) * 0x0080401;
            else
                v = (x + time / 16) * 0x0080401;
            v &= 0x00ffffff;

            /* cross if compositor uses X from XRGB as alpha */
            if (abs(x - y) > 6 && abs(x + y - height) > 6)
                v |= 0xff000000;

            *pixel++ = v;
        }

        pixel += padding;
    }
}

static const struct wl_callback_listener frame_listener;

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    struct window *window = data;
    struct buffer *buffer;

    buffer = window_next_buffer(window);
    if (!buffer) {
        fprintf(stderr,
                !callback ? "Failed to create the first buffer.\n" :
                "Both buffers busy at redraw(). Server bug?\n");
        abort();
    }

    paint_pixels(buffer->shm_data, 20, window->width, window->height, time);

    wl_surface_attach(window->surface, buffer->buffer, 0, 0);
    wl_surface_damage(window->surface,
                      20, 20, window->width - 40, window->height - 40);

    if (callback)
        wl_callback_destroy(callback);

    window->callback = wl_surface_frame(window->surface);
    wl_callback_add_listener(window->callback, &frame_listener, window);
    wl_surface_commit(window->surface);
    buffer->busy = 1;
}

static const struct wl_callback_listener frame_listener = {
    redraw
};

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    struct display *d = data;

    d->formats |= (1 << format);
}

struct wl_shm_listener shm_listener = {
    shm_format
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
                       uint32_t id, const char *interface, uint32_t version)
{
    struct display *d = data;

    if (strcmp(interface, "wl_compositor") == 0) {
        d->compositor =
            wl_registry_bind(registry,
                             id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        d->shm = wl_registry_bind(registry,
                                  id, &wl_shm_interface, 1);
        wl_shm_add_listener(d->shm, &shm_listener, d);
    } else if (strcmp(interface, "wl_shell") == 0) {
        d->shell = wl_registry_bind(registry,
                                    id, &wl_shell_interface, 1);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

static struct display *
create_display(const char *display_name)
{
    struct display *display;

    display = malloc(sizeof *display);
    if (display == NULL) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    display->display = wl_display_connect(display_name);
    assert(display->display);

    display->formats = 0;
    display->registry = wl_display_get_registry(display->display);
    wl_registry_add_listener(display->registry,
                             &registry_listener, display);
    wl_display_roundtrip(display->display);
    if (display->shm == NULL) {
        fprintf(stderr, "No wl_shm global\n");
        exit(1);
    }

    wl_display_roundtrip(display->display);

    /*
     * Why do we need two roundtrips here?
     *
     * wl_display_get_registry() sends a request to the server, to which
     * the server replies by emitting the wl_registry.global events.
     * The first wl_display_roundtrip() sends wl_display.sync. The server
     * first processes the wl_display.get_registry which includes sending
     * the global events, and then processes the sync. Therefore when the
     * sync (roundtrip) returns, we are guaranteed to have received and
     * processed all the global events.
     *
     * While we are inside the first wl_display_roundtrip(), incoming
     * events are dispatched, which causes registry_handle_global() to
     * be called for each global. One of these globals is wl_shm.
     * registry_handle_global() sends wl_registry.bind request for the
     * wl_shm global. However, wl_registry.bind request is sent after
     * the first wl_display.sync, so the reply to the sync comes before
     * the initial events of the wl_shm object.
     *
     * The initial events that get sent as a reply to binding to wl_shm
     * include wl_shm.format. These tell us which pixel formats are
     * supported, and we need them before we can create buffers. They
     * don't change at runtime, so we receive them as part of init.
     *
     * When the reply to the first sync comes, the server may or may not
     * have sent the initial wl_shm events. Therefore we need the second
     * wl_display_roundtrip() call here.
     *
     * The server processes the wl_registry.bind for wl_shm first, and
     * the second wl_display.sync next. During our second call to
     * wl_display_roundtrip() the initial wl_shm events are received and
     * processed. Finally, when the reply to the second wl_display.sync
     * arrives, it guarantees we have processed all wl_shm initial events.
     *
     * This sequence contains two examples on how wl_display_roundtrip()
     * can be used to guarantee, that all reply events to a request
     * have been received and processed. This is a general Wayland
     * technique.
     */

    if (!(display->formats & (1 << WL_SHM_FORMAT_XRGB8888))) {
        fprintf(stderr, "WL_SHM_FORMAT_XRGB32 not available\n");
        exit(1);
    }

    return display;
}

static void
destroy_display(struct display *display)
{
    if (display->shm)
        wl_shm_destroy(display->shm);

    if (display->shell)
        wl_shell_destroy(display->shell);

    if (display->compositor)
        wl_compositor_destroy(display->compositor);

    wl_registry_destroy(display->registry);
    wl_display_flush(display->display);
    wl_display_disconnect(display->display);
    free(display);
}

static void
signal_int(int signum)
{
    running = 0;
}

int
main(int argc, char **argv)
{
    struct sigaction sigint;
    struct display *display;
    struct window *window;
    int ret = 0;
    const char *display_name;

    if (argc == 1)
        display_name = NULL;
    else if (argc == 2)
    {
        display_name = argv[1];
    }
    else
    {
        printf("usage: simple-shm DISPLAY_NAME");
        return 1;
    }

    display = create_display(display_name);
    window = create_window(display, 250, 250);
    if (!window)
        return 1;

    sigint.sa_handler = signal_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sigint, NULL);

    /* Initialise damage to full surface, so the padding gets painted */
    wl_surface_damage(window->surface, 0, 0,
                      window->width, window->height);

    redraw(window, NULL, 0);

    while (running && ret != -1)
        ret = wl_display_dispatch(display->display);

    fprintf(stderr, "simple-shm exiting\n");

    destroy_window(window);
    destroy_display(display);

    return 0;
}
