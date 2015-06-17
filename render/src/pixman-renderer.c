#include "pepper-pixman-renderer.h"
#include "pepper-render-internal.h"
#include <stdlib.h>

typedef struct pixman_renderer      pixman_renderer_t;
typedef struct pixman_surface_state pixman_surface_state_t;
typedef struct pixman_render_target pixman_render_target_t;

struct pixman_render_target
{
    pepper_render_target_t  base;
    pixman_image_t         *image;
};

struct pixman_renderer
{
    pepper_renderer_t   base;
};

struct pixman_surface_state
{
    pixman_renderer_t  *renderer;

    pepper_object_t    *surface;
    pepper_object_t    *buffer;
    int                 buffer_width, buffer_height;
    pixman_image_t     *image;

    struct wl_listener  buffer_destroy_listener;
    struct wl_listener  surface_destroy_listener;
};

static inline pixman_format_code_t
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
pixman_renderer_destroy(pepper_renderer_t *renderer)
{
    free(renderer);
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
    pixman_surface_state_t *state =
        pepper_container_of(listener, pixman_surface_state_t, surface_destroy_listener);

    surface_state_release_buffer(state);
    wl_list_remove(&state->surface_destroy_listener.link);
    pepper_object_set_user_data(state->surface, state->renderer, NULL, NULL);
    free(state);
}

static void
surface_state_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
    pixman_surface_state_t *state =
        pepper_container_of(listener, pixman_surface_state_t, buffer_destroy_listener);

    surface_state_release_buffer(state);
}

static pixman_surface_state_t *
get_surface_state(pepper_renderer_t *renderer, pepper_object_t *surface)
{
    pixman_surface_state_t *state = pepper_object_get_user_data(surface, renderer);

    if (!state)
    {
        state = calloc(1, sizeof(pixman_surface_state_t));
        if (!state)
            return NULL;

        state->surface = surface;
        state->buffer_destroy_listener.notify = surface_state_handle_buffer_destroy;
        state->surface_destroy_listener.notify = surface_state_handle_surface_destroy;

        pepper_object_add_destroy_listener(surface, &state->surface_destroy_listener);
        pepper_object_set_user_data(surface, renderer, state, NULL);
    }

    return state;
}

static pepper_bool_t
surface_state_attach_shm(pixman_surface_state_t *state, pepper_object_t *buffer)
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

static pepper_bool_t
pixman_renderer_attach_surface(pepper_renderer_t *renderer, pepper_object_t *surface,
                               int *w, int *h)
{
    pixman_surface_state_t *state = get_surface_state(renderer, surface);
    pepper_object_t        *buffer = pepper_surface_get_buffer(surface);

    if (!buffer)
    {
        *w = 0;
        *h = 0;

        surface_state_release_buffer(state);
        return PEPPER_TRUE;
    }

    if (surface_state_attach_shm(state, buffer))
        goto done;

    /* TODO: Other buffer types which can be mapped into CPU address space. i.e. wl_tbm. */

    return PEPPER_FALSE;

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
    pepper_object_add_destroy_listener(buffer, &state->buffer_destroy_listener);

    /* Output buffer size info. */
    *w = state->buffer_width;
    *h = state->buffer_height;

    return PEPPER_TRUE;
}

static pepper_bool_t
pixman_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_object_t *surface)
{
    return PEPPER_TRUE;
}

static pepper_bool_t
pixman_renderer_read_pixels(pepper_renderer_t *renderer,
                            int x, int y, int w, int h,
                            void *pixels, pepper_format_t format)
{
    pixman_image_t         *src = ((pixman_render_target_t *)renderer->target)->image;
    pixman_image_t         *dst;
    pixman_format_code_t    pixman_format;
    int                     stride;

    if (!src)
        return PEPPER_FALSE;

    pixman_format = get_pixman_format(format);

    if (!pixman_format)
    {
        /* PEPPER_ERROR("Invalid format.\n"); */
        return PEPPER_FALSE;
    }

    stride = (PEPPER_FORMAT_BPP(format) / 8) * w;
    dst = pixman_image_create_bits(pixman_format, w, h, pixels, stride);

    if (!dst)
    {
        /* PEPPER_ERROR("Failed to create pixman image.\n"); */
        return PEPPER_FALSE;
    }

    pixman_image_composite(PIXMAN_OP_SRC, src, NULL, dst, x, y, 0, 0, 0, 0, w, h);
    pixman_image_unref(dst);

    return PEPPER_TRUE;
}

static void
pixman_renderer_repaint_output(pepper_renderer_t *renderer, pepper_object_t *output,
                               const pixman_region32_t *output_damage)
{
    pixman_image_t *dst = ((pixman_render_target_t *)renderer->target)->image;

    if (dst && pixman_region32_not_empty((pixman_region32_t*)output_damage))
    {
        pixman_fill(pixman_image_get_data(dst),
                    pixman_image_get_stride(dst) / sizeof(uint32_t),
                    PIXMAN_FORMAT_BPP(pixman_image_get_format(dst)),
                    0, 0,
                    pixman_image_get_width(dst),
                    pixman_image_get_height(dst),
                    0x00000000);
    }

    /* TODO: */
}

PEPPER_API pepper_renderer_t *
pepper_pixman_renderer_create(pepper_object_t *compositor)
{
    pixman_renderer_t    *renderer;

    renderer = calloc(1, sizeof(pixman_renderer_t));
    if (!renderer)
        return NULL;

    renderer->base.compositor = compositor;

    /* Backend functions. */
    renderer->base.destroy              = pixman_renderer_destroy;
    renderer->base.attach_surface       = pixman_renderer_attach_surface;
    renderer->base.flush_surface_damage = pixman_renderer_flush_surface_damage;
    renderer->base.read_pixels          = pixman_renderer_read_pixels;
    renderer->base.repaint_output       = pixman_renderer_repaint_output;

    return &renderer->base;
}

static void
pixman_render_target_destroy(pepper_render_target_t *target)
{
    pixman_render_target_t *pt = (pixman_render_target_t *)target;

    if (pt->image)
        pixman_image_unref(pt->image);

    free(target);
}

PEPPER_API pepper_render_target_t *
pepper_pixman_renderer_create_target(pepper_format_t format, void *pixels,
                                     int stride, int width, int height)
{
    pixman_render_target_t *target;
    pixman_format_code_t    pixman_format;

    target = calloc(1, sizeof(pixman_render_target_t));
    if (!target)
        return NULL;

    target->base.destroy    = pixman_render_target_destroy;

    pixman_format = get_pixman_format(format);
    if (!pixman_format)
        goto error;

    target->image = pixman_image_create_bits(pixman_format, width, height, pixels, stride);
    if (!target->image)
        goto error;

    target->base.destroy = pixman_render_target_destroy;
    return &target->base;

error:
    if (target)
        free(target);

    return NULL;
}
