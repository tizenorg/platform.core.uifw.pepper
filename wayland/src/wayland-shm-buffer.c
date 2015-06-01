#include "wayland-internal.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

static int
set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

err:
    close(fd);
    return -1;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
    int fd;

#ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
#else
    fd = mkstemp(tmpname);
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
#endif

    return fd;
}

int
create_anonymous_file(off_t size)
{
    static const char template[] = "/pepper-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;
    int ret;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = malloc(strlen(path) + sizeof(template));
    if (!name)
        return -1;

    strcpy(name, path);
    strcat(name, template);

    fd = create_tmpfile_cloexec(name);

    free(name);

    if (fd < 0)
        return -1;

#ifdef HAVE_POSIX_FALLOCATE
    ret = posix_fallocate(fd, 0, size);
    if (ret != 0) {
        close(fd);
        errno = ret;
        return -1;
    }
#else
    ret = ftruncate(fd, size);
    if (ret < 0) {
        close(fd);
        return -1;
    }
#endif

    return fd;
}
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

    fd = create_anonymous_file(buffer->size);

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
