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
    int                 buffer_width, buffer_height;
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

/* TODO: Similar with gl renderer. There might be a way of reusing those codes.  */

static void
surface_state_release_buffer(pixman_surface_state_t *state)
{
    if (state->image)
    {
        pixman_image_unref(state->image);
        state->image = NULL;
    }

    if (state->buffer)
    {
        pepper_buffer_unreference(state->buffer);
        state->buffer = NULL;

        wl_list_remove(&state->buffer_destroy_listener.link);
    }
}

static void
surface_state_handle_surface_destroy(struct wl_listener *listener, void *data)
{
    pixman_surface_state_t *state = wl_container_of(listener, state, surface_destroy_listener);

    surface_state_release_buffer(state);
    wl_list_remove(&state->surface_destroy_listener.link);
    pepper_surface_set_user_data(state->surface, state->renderer, NULL, NULL);
    pepper_free(state);
}

static void
surface_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
    pixman_surface_state_t *state = wl_container_of(listener, state, buffer_destroy_listener);

    surface_state_release_buffer(state);
}

static pixman_surface_state_t *
get_surface_state(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    pixman_surface_state_t *state = pepper_surface_get_user_data(surface, renderer);

    if (!state)
    {
        state = pepper_calloc(1, sizeof(pixman_surface_state_t));
        if (!state)
            return NULL;

        state->surface = surface;
        state->buffer_destroy_listener.notify = surface_state_handle_buffer_destroy;
        state->surface_destroy_listener.notify = surface_state_handle_surface_destroy;

        pepper_surface_add_destroy_listener(surface, &state->surface_destroy_listener);
        pepper_surface_set_user_data(surface, renderer, state, NULL);
    }

    return state;
}

static pepper_bool_t
surface_state_attach_shm(pixman_surface_state_t *state, pepper_buffer_t *buffer)
{
    struct wl_shm_buffer   *shm_buffer = wl_shm_buffer_get(pepper_buffer_get_resource(buffer));
    pixman_format_code_t    format;
    int                     w, h;
    pixman_image_t         *image;

    if (!shm_buffer)
        return PEPPER_FALSE;

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
        return PEPPER_FALSE;
    }

    w = wl_shm_buffer_get_width(shm_buffer);
    h = wl_shm_buffer_get_height(shm_buffer);

    image = pixman_image_create_bits(format, w, h,
                                     wl_shm_buffer_get_data(shm_buffer),
                                     wl_shm_buffer_get_stride(shm_buffer));

    if (!image)
        return PEPPER_FALSE;

    state->buffer_width = w;
    state->buffer_height = h;
    state->image = image;

    return PEPPER_TRUE;;
}

static void
pixman_renderer_attach_surface(pepper_renderer_t *renderer, pepper_surface_t *surface,
                               int *w, int *h)
{
    pixman_surface_state_t *state = get_surface_state(renderer, surface);
    pepper_buffer_t        *buffer = pepper_surface_get_buffer(surface);

    if (!buffer)
    {
        surface_state_release_buffer(state);
        return;
    }

    if (surface_state_attach_shm(state, buffer))
        goto done;

    /* TODO: Other buffer types which can be mapped into CPU address space. i.e. wl_tbm. */

    /* TODO: return error so that the compositor can handle that error. */
    PEPPER_ASSERT(PEPPER_FALSE);
    return;

done:
    pepper_buffer_reference(buffer);

    /* Release previous buffer. */
    if (state->buffer)
    {
        pepper_buffer_unreference(state->buffer);
        wl_list_remove(&state->buffer_destroy_listener.link);
    }

    /* Set new buffer. */
    state->buffer = buffer;
    pepper_buffer_add_destroy_listener(buffer, &state->buffer_destroy_listener);

    /* Output buffer size info. */
    *w = state->buffer_width;
    *h = state->buffer_height;
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
