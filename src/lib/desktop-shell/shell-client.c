#include <wayland-client.h>
#include <protocol/pepper-shell-client-protocol.h>
#include <pepper-utils.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define CURSOR_WIDTH    32
#define CURSOR_HEIGHT   32

struct buffer
{
    struct wl_buffer    *buffer;
    int                  w, h, size;
    void                *data;
};

struct cursor
{
    struct wl_surface   *surface;

    struct wl_callback  *frame_callback;
    int                  frame_done;
    int                  num_buffers;
    int                  current_buffer;
    struct buffer        buffers[4];
};

struct display
{
    struct wl_display       *display;
    struct wl_registry      *registry;

    struct wl_compositor    *compositor;
    struct wl_shm           *shm;
    struct pepper_shell     *shell;
    int                      run;

    struct cursor        cursor;
};

static void
handle_global(void *data, struct wl_registry *registry, uint32_t id,
              const char *interface, uint32_t version)
{
    struct display *display = data;

    if (strcmp(interface, "wl_compositor") == 0)
        display->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    else if (strcmp(interface, "wl_shm") == 0)
        display->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    else if (strcmp(interface, "pepper_shell") == 0)
        display->shell = wl_registry_bind(registry, id, &pepper_shell_interface, 1);
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener =
{
    handle_global,
    handle_global_remove,
};

static void
frame_done(void *data, struct wl_callback *callback, uint32_t time)
{
    struct cursor *cursor = data;

    cursor->frame_done = 1;

    /* TODO: Redraw cursor */
}

static const struct wl_callback_listener frame_listener =
{
    frame_done
};

static int
create_shm_buffer(struct display *display, struct buffer *buffer, int w, int h, uint32_t format)
{
    struct wl_shm_pool *pool;
    int                 fd, size, stride;
    void               *data;

    stride = w * 4;
    size = stride * h;

    fd = pepper_create_anonymous_file(size);
    if (fd < 0)
    {
        fprintf(stderr, "pepper_create_anonymous_file() failed.\n");
        return -1;
    }

    data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd ,0);
    if (data == MAP_FAILED)
    {
        fprintf(stderr, "mmap() failed.\n");
        close(fd);
        return -1;
    }

    pool = wl_shm_create_pool(display->shm, fd, size);
    buffer->buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride, format);
    wl_shm_pool_destroy(pool);
    close(fd);

    buffer->data = data;
    buffer->w = w;
    buffer->h = h;
    buffer->size = size;

    return 0;
}

static int
init_cursor(struct cursor *cursor, struct display *display)
{
    cursor->surface = wl_compositor_create_surface(display->compositor);
    if (!cursor->surface)
    {
        fprintf(stderr, "wl_compositor_create_surface() failed.\n");
        return -1;
    }

    if (!create_shm_buffer(display, &cursor->buffers[0],
                           CURSOR_WIDTH, CURSOR_HEIGHT, WL_SHM_FORMAT_XRGB8888))
    {
        fprintf(stderr, "create_shm_buffer() failed.\n");
        return -1;
    }

    /* TODO: Draw the initial cursor image. */
    memset(cursor->buffers[0].data, 0xff, cursor->buffers[0].size);

    /* Update the surface with the initial buffer. */
    wl_surface_attach(cursor->surface, cursor->buffers[0].buffer, 0, 0);
    wl_surface_damage(cursor->surface, 0, 0, cursor->buffers[0].w, cursor->buffers[0].h);
    cursor->frame_callback = wl_surface_frame(cursor->surface);
    wl_callback_add_listener(cursor->frame_callback, &frame_listener, cursor);
    wl_surface_commit(cursor->surface);

    cursor->frame_done = 0;
    pepper_shell_set_cursor(display->shell, cursor->surface);

    return 0;
}

static void
fini_cursor(struct cursor *cursor)
{
    int i;

    wl_surface_destroy(cursor->surface);

    for (i = 0; i < cursor->num_buffers; i++)
        wl_buffer_destroy(cursor->buffers[i].buffer);
}

int
main(int argc, char **argv)
{
    struct display  display;
    int             ret;

    memset(&display, 0x00, sizeof(struct display));

    if (argc != 2)
    {
        fprintf(stderr, "socket name was not given.\n");
        return -1;
    }

    display.display = wl_display_connect(argv[0]);
    if (!display.display)
    {
        fprintf(stderr, "wl_display_connect(%s) failed\n", argv[0]);
        return -1;
    }

    display.registry = wl_display_get_registry(display.display);
    wl_registry_add_listener(display.registry, &registry_listener, &display);
    wl_display_roundtrip(display.display);

    if (display.shm)
    {
        fprintf(stderr, "Couldn't find wl_shm\n");
        return -1;
    }

    if (display.compositor)
    {
        fprintf(stderr, "Couldn't find wl_compositor\n");
        return -1;
    }

    if (display.shell)
    {
        fprintf(stderr, "Couldn't find pepper_shell\n");
        return -1;
    }

    init_cursor(&display.cursor, &display);

    display.run = 1;
    ret         = 0;

    while (display.run && ret != -1)
        ret = wl_display_dispatch(display.display);

    fini_cursor(&display.cursor);

    wl_shm_destroy(display.shm);
    wl_compositor_destroy(display.compositor);
    pepper_shell_destroy(display.shell);
    wl_display_disconnect(display.display);

    return 0;
}
