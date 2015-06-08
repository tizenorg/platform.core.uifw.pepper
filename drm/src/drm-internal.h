#ifndef DRM_INTERNAL_H
#define DRM_INTERNAL_H

#include <pixman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <pepper-libinput.h>
#include <pepper-render.h>

#include "pepper-drm.h"

#define DUMB_FB_COUNT   2

/* TODO: Error Logging. */
#define PEPPER_ERROR(...)

typedef struct drm_output       drm_output_t;
typedef struct drm_fb           drm_fb_t;

struct pepper_drm
{
    pepper_compositor_t        *compositor;
    pepper_libinput_t          *input;

    struct wl_list              output_list;

    uint32_t                   *crtcs;
    uint32_t                    count_crtcs;

    uint32_t                    min_width, min_height;
    uint32_t                    max_width, max_height;

    int                         drm_fd;
    int                         drm_sysnum;
    struct wl_event_source     *drm_event_source;

    struct udev                *udev;
    struct udev_monitor        *udev_monitor;
    struct wl_event_source     *udev_monitor_source;
};

struct drm_output
{
    pepper_drm_t               *drm;
    pepper_output_t            *base;

    struct wl_list              link;

    int32_t                     subpixel;
    uint32_t                    w, h;

    uint32_t                    crtc_id;
    uint32_t                    conn_id;

    struct wl_signal            destroy_signal;
    struct wl_signal            mode_change_signal;
    struct wl_signal            frame_signal;

    int                         mode_count;
    drmModeModeInfo            *modes;
    drmModeModeInfo            *current_mode;

    drmModeCrtc                *saved_crtc;

    drm_fb_t                   *dumb_fb[DUMB_FB_COUNT];
    pixman_image_t             *dumb_image[DUMB_FB_COUNT];
    int                         back_fb_index;

    struct gbm_device          *gbm_device;
    struct gbm_surface         *gbm_surface;

    drm_fb_t                   *front_fb;
    drm_fb_t                   *back_fb;

    pepper_renderer_t          *renderer;

    pepper_bool_t               vblank_pending;
    pepper_bool_t               page_flip_pending;

    /* TODO */
};

struct drm_fb
{
    drm_output_t               *output;

    int                         fd;
    uint32_t                    id;
    uint32_t                    handle;
    uint32_t                    stride;
    uint32_t                    size;

    struct gbm_bo              *bo;
    void                       *map;
};

pepper_bool_t
pepper_drm_output_create(pepper_drm_t *drm);

void
pepper_drm_output_destroy(drm_output_t *output);

#endif /* DRM_INTERNAL_H */
