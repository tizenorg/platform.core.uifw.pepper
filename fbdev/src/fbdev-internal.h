#ifndef FBDEV_INTERNAL_H
#define FBDEV_INTERNAL_H

#include <pixman.h>

#include <pepper-libinput.h>
#include <pepper-render.h>

#include "pepper-fbdev.h"

/* TODO: Error Logging. */
#define PEPPER_ERROR(...)

typedef struct fbdev_output     fbdev_output_t;

struct pepper_fbdev
{
    pepper_compositor_t        *compositor;
    pepper_libinput_t          *input;

    struct wl_list              output_list;

    uint32_t                    min_width, min_height;
    uint32_t                    max_width, max_height;

    struct udev                *udev;
};

struct fbdev_output
{
    pepper_fbdev_t             *fbdev;
    pepper_output_t            *base;

    struct wl_list              link;

    int32_t                     subpixel;
    uint32_t                    w, h;
    uint32_t                    pixel_format;
    uint32_t                    bits_per_pixel;

    struct wl_signal            destroy_signal;
    struct wl_signal            mode_change_signal;
    struct wl_signal            frame_signal;

    void                       *fb;
    pixman_image_t             *fb_image;

    pepper_renderer_t          *renderer;
    /* TODO */
};

pepper_bool_t
pepper_fbdev_output_create(pepper_fbdev_t *fbdev);

void
pepper_fbdev_output_destroy(fbdev_output_t *output);

#endif /* FBDEV_INTERNAL_H */
