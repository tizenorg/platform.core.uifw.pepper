#include "wayland-internal.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pepper-utils.h>

static void
buffer_release(void *data, struct wl_buffer *buf)
{
    wayland_shm_buffer_t *buffer = data;

    if (buffer->output)
    {
        /* Move to free buffer list. */
        wl_list_remove(&buffer->link);
        wl_list_insert(buffer->output->shm.free_buffers.next, &buffer->link);
    }
    else
    {
        /* Orphaned buffer due to output resize or something. Destroy it. */
        wayland_shm_buffer_destroy(buffer);
    }
}

static const struct wl_buffer_listener buffer_listener =
{
    buffer_release,
};

wayland_shm_buffer_t *
wayland_shm_buffer_create(wayland_output_t *output)
{
    wayland_shm_buffer_t   *buffer;
    int                     fd;
    struct wl_shm_pool     *pool;

    buffer = calloc(1, sizeof(wayland_shm_buffer_t));
    if (!buffer)
        return NULL;

    buffer->output = output;
    wl_list_init(&buffer->link);

    buffer->w = output->w;
    buffer->h = output->h;
    buffer->stride  = buffer->w * 4;
    buffer->size    = buffer->stride * buffer->h;

    fd = pepper_create_anonymous_file(buffer->size);

    if (fd < 0)
    {
        PEPPER_ERROR("Failed to create anonymous file");
        goto error;
    }

    buffer->pixels = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!buffer->pixels)
    {
        PEPPER_ERROR("mmap() failed for fd=%d\n", fd);
        goto error;
    }

    pool = wl_shm_create_pool(output->conn->shm, fd, buffer->size);
    buffer->buffer = wl_shm_pool_create_buffer(pool, 0, buffer->w, buffer->h,
                                               buffer->stride, WL_SHM_FORMAT_ARGB8888);
    wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
    wl_shm_pool_destroy(pool);
    close(fd);

    buffer->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, buffer->w, buffer->h,
                                             buffer->pixels, buffer->stride);
    pixman_region32_init_rect(&buffer->damage, 0, 0, buffer->w, buffer->h);

    return buffer;

error:
    if (fd >= 0)
        close(fd);

    if (buffer)
        free(buffer);

    return NULL;
}

void
wayland_shm_buffer_destroy(wayland_shm_buffer_t *buffer)
{
    pixman_region32_fini(&buffer->damage);
    pixman_image_unref(buffer->image);
    wl_buffer_destroy(buffer->buffer);
    munmap(buffer->pixels, buffer->size);
    wl_list_remove(&buffer->link);
}
