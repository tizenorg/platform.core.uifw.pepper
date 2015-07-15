#ifndef FBDEV_INTERNAL_H
#define FBDEV_INTERNAL_H

#include <pixman.h>

#include <pepper-libinput.h>
#include <pepper-render.h>
#include <pepper-pixman-renderer.h>

#include "pepper-fbdev.h"

/* TODO: Error Logging. */
#define PEPPER_ERROR(...)

typedef struct fbdev_output     fbdev_output_t;

struct pepper_fbdev
{
    pepper_object_t            *compositor;
    pepper_libinput_t          *input;

    struct wl_list              output_list;

    uint32_t                    min_width, min_height;
    uint32_t                    max_width, max_height;

    struct udev                *udev;

    pepper_renderer_t          *pixman_renderer;
};

struct fbdev_output
{
    pepper_fbdev_t             *fbdev;
    pepper_object_t            *base;

    struct wl_list              link;

    pepper_renderer_t          *renderer;

    pepper_render_target_t     *render_target;
    pepper_format_t             format;
    int32_t                     subpixel;
    int                         w, h;
    int                         bpp;
    int                         stride;

    void                       *frame_buffer_pixels;
    pixman_image_t             *frame_buffer_image;
    pixman_image_t             *shadow_image;
    pepper_bool_t               use_shadow;

    struct wl_signal            destroy_signal;
    struct wl_signal            mode_change_signal;
    struct wl_signal            frame_signal;

    pepper_object_t            *primary_plane;
    /* TODO */
};

pepper_bool_t
pepper_fbdev_output_create(pepper_fbdev_t *fbdev, const char *renderer);

void
pepper_fbdev_output_destroy(fbdev_output_t *output);

#endif /* FBDEV_INTERNAL_H */
