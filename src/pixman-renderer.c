#include "pepper-pixman-renderer.h"
#include "common.h"
#include <pixman.h>

typedef struct pixman_renderer      pixman_renderer_t;
typedef struct pixman_surface_state pixman_surface_state_t;

struct pixman_renderer
{
    pepper_renderer_t   base;
};

struct pixman_surface_state
{
    pixman_renderer_t  *renderer;

    pepper_surface_t   *surface;
    pepper_buffer_t    *buffer;
    pixman_image_t     *image;

    struct wl_listener  buffer_destroy_listener;
    struct wl_listener  surface_destroy_listener;
};

static PEPPER_INLINE pixman_format_code_t
get_pixman_format(pepper_format_t format)
{
    switch (format)
    {
    case PEPPER_FORMAT_ARGB8888:
        return PIXMAN_a8r8g8b8;
    case PEPPER_FORMAT_XRGB8888:
        return PIXMAN_x8r8g8b8;
    case PEPPER_FORMAT_RGB888:
        return PIXMAN_r8g8b8;
    case PEPPER_FORMAT_RGB565:
        return PIXMAN_r5g6b5;
    case PEPPER_FORMAT_ABGR8888:
        return PIXMAN_a8b8g8r8;
    case PEPPER_FORMAT_XBGR8888:
        return PIXMAN_x8b8g8r8;
    case PEPPER_FORMAT_BGR888:
        return PIXMAN_b8g8r8;
    case PEPPER_FORMAT_BGR565:
        return PIXMAN_b5g6r5;
    case PEPPER_FORMAT_ALPHA:
        return PIXMAN_a8;
    default:
        break;
    }

    return (pixman_format_code_t)0;
}

static void
pixman_renderer_destroy(pepper_renderer_t *r)
{
    pixman_renderer_t *renderer = (pixman_renderer_t *)r;
    pepper_free(renderer);
}

static pepper_bool_t
pixman_renderer_read_pixels(pepper_renderer_t *r, void *target,
                            int x, int y, int w, int h,
                            void *pixels, pepper_format_t format)
{
    pixman_image_t         *image = (pixman_image_t *)target;
    pixman_image_t         *dst;
    pixman_format_code_t    pixman_format;
    int                     stride;

    if (!image)
        return PEPPER_FALSE;

    pixman_format = get_pixman_format(format);

    if (!pixman_format)
    {
        PEPPER_ERROR("Invalid format.\n");
        return PEPPER_FALSE;
    }

    stride = (PEPPER_FORMAT_BPP(format) / 8) * w;
    dst = pixman_image_create_bits(pixman_format, w, h, pixels, stride);

    if (!dst)
    {
        PEPPER_ERROR("Failed to create pixman image.\n");
        return PEPPER_FALSE;
    }

    pixman_image_composite(PIXMAN_OP_SRC, image, NULL, dst, x, y, 0, 0, 0, 0, w, h);
    return PEPPER_TRUE;
}

static void
release_current_buffer(pixman_surface_state_t *state)
{
    if (!state->buffer)
        return;

    pepper_buffer_unreference(state->buffer);
    pixman_image_unref(state->image);
    wl_list_remove(&state->buffer_destroy_listener.link);
    wl_list_remove(&state->surface_destroy_listener.link);

    state->buffer = NULL;
    state->image = NULL;
}

static void
surface_state_handle_surface_destroy(struct wl_listener *listener, void *data)
{
    pixman_surface_state_t *state = wl_container_of(listener, state, surface_destroy_listener);

    release_current_buffer(state);
    pepper_free(state);
}

static void
surface_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
    pixman_surface_state_t *state = wl_container_of(listener, state, buffer_destroy_listener);

    release_current_buffer(state);
}

static void
pixman_renderer_attach_surface(pepper_renderer_t *r, pepper_surface_t *surface, int *w, int *h)
{
    pixman_surface_state_t *state;
    struct wl_shm_buffer   *shm_buffer;
    struct wl_resource     *resource;

    state = pepper_surface_get_user_data(surface, r);

    if (!state)
    {
        state = pepper_calloc(1, sizeof(pixman_surface_state_t));
        if (!state)
            return;

        state->surface = surface;
        state->buffer_destroy_listener.notify = surface_state_handle_buffer_destroy;
        state->surface_destroy_listener.notify = surface_state_handle_surface_destroy;

        pepper_surface_add_destroy_listener(surface, &state->surface_destroy_listener);
    }

    /* Release previously attached buffer. */
    release_current_buffer(state);

    state->buffer = pepper_surface_get_buffer(surface);
    if (!state->buffer)
        return;

    resource = pepper_buffer_get_resource(state->buffer);

    if ((shm_buffer = wl_shm_buffer_get(resource)) != NULL)
    {
        pixman_format_code_t    format;
        int                     width, height;

        switch (wl_shm_buffer_get_format(shm_buffer))
        {
        case WL_SHM_FORMAT_XRGB8888:
            format = PIXMAN_x8r8g8b8;
            break;
        case WL_SHM_FORMAT_ARGB8888:
            format = PIXMAN_a8r8g8b8;
            break;
        case WL_SHM_FORMAT_RGB565:
            format = PIXMAN_r5g6b5;
            break;
        default:
            PEPPER_ERROR("Unknown shm buffer format.\n");
            return;
        }

        width = wl_shm_buffer_get_width(shm_buffer);
        height = wl_shm_buffer_get_height(shm_buffer);

        pepper_buffer_add_destroy_listener(state->buffer, &state->buffer_destroy_listener);
        state->image = pixman_image_create_bits(format, width, height,
                                                wl_shm_buffer_get_data(shm_buffer),
                                                wl_shm_buffer_get_stride(shm_buffer));

        *w = width;
        *h = height;

        PEPPER_ASSERT(state->image);
        return;
    }

    /* TODO: Other buffer types which can be mapped into CPU address space. i.e. wl_tbm. */
}

static void
pixman_renderer_draw(pepper_renderer_t *r, void *target, void *data)
{
    pixman_image_t     *image = (pixman_image_t *)target;

    if (image)
    {
        /* TODO: */
        pixman_fill(pixman_image_get_data(image),
                    pixman_image_get_stride(image) / sizeof(uint32_t),
                    PIXMAN_FORMAT_BPP(pixman_image_get_format(image)),
                    0, 0,
                    pixman_image_get_width(image),
                    pixman_image_get_height(image),
                    0xffffffff);
    }
}

PEPPER_API pepper_renderer_t *
pepper_pixman_renderer_create()
{
    pixman_renderer_t    *renderer;

    renderer = pepper_calloc(1, sizeof(pixman_renderer_t));
    if (!renderer)
        return NULL;

    renderer->base.destroy              = pixman_renderer_destroy;
    renderer->base.read_pixels          = pixman_renderer_read_pixels;
    renderer->base.attach_surface       = pixman_renderer_attach_surface;
    renderer->base.draw                 = pixman_renderer_draw;

    return &renderer->base;
}
