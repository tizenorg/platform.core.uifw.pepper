#include "pepper-pixman-renderer.h"
#include "common.h"
#include <pixman.h>

typedef struct pixman_renderer  pixman_renderer_t;

struct pixman_renderer
{
    pepper_renderer_t   base;
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
    renderer->base.draw                 = pixman_renderer_draw;

    return &renderer->base;
}
