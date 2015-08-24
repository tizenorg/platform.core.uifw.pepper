#ifndef DRM_INTERNAL_H
#define DRM_INTERNAL_H

#include <pixman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <pepper-output-backend.h>
#include <pepper-libinput.h>
#include <pepper-render.h>

#include "pepper-drm.h"

#define DUMB_FB_COUNT   2

typedef struct drm_output       drm_output_t;
typedef struct drm_fb           drm_fb_t;
typedef struct drm_plane        drm_plane_t;

struct pepper_drm
{
    pepper_compositor_t        *compositor;

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

    struct gbm_device          *gbm_device;

    char                       *renderer;
    pepper_renderer_t          *pixman_renderer;
    pepper_renderer_t          *gl_renderer;

    pepper_list_t               plane_list;
};

struct drm_output
{
    pepper_drm_t               *drm;
    pepper_output_t            *base;
    char                        name[32];

    struct wl_list              link;

    int32_t                     subpixel;
    uint32_t                    w, h;

    int32_t                     crtc_index;
    uint32_t                    crtc_id;
    uint32_t                    conn_id;

    int                         mode_count;
    drmModeModeInfo            *modes;
    drmModeModeInfo            *current_mode;

    drmModeCrtc                *saved_crtc;

    struct gbm_surface         *gbm_surface;

    drm_fb_t                   *front_fb;
    drm_fb_t                   *back_fb;

    pepper_renderer_t          *renderer;
    pepper_render_target_t     *render_target;
    pepper_render_target_t     *gl_render_target;

    pepper_bool_t               use_pixman;
    drm_fb_t                   *dumb_fb[DUMB_FB_COUNT];
    int                         back_fb_index;

    pepper_bool_t               vblank_pending;
    pepper_bool_t               page_flip_pending;

    pepper_view_t              *cursor_view;
    pepper_plane_t             *cursor_plane;
    pepper_plane_t             *primary_plane;

    /* TODO */
};

struct drm_fb
{
    drm_output_t               *output;

    int                         fd;
    uint32_t                    id;
    uint32_t                    handle;

    uint32_t                    w, h;
    uint32_t                    stride;
    uint32_t                    size;

    struct gbm_bo              *bo;
    void                       *map;
    pepper_render_target_t     *target;
};

struct drm_plane
{
    pepper_drm_t               *drm;
    pepper_plane_t             *base;
    drm_output_t               *output;

    uint32_t                    possible_crtcs;
    uint32_t                    plane_id;

    drm_fb_t                   *front_fb;
    drm_fb_t                   *back_fb;

    int                         sx, sy, sw, sh; /* src *//* FIXME: uint32_t? */
    int                         dx, dy, dw, dh; /* dst *//* FIXME: uint32_t? */

    /* TODO */

    pepper_list_t               link;
};

pepper_bool_t
pepper_drm_output_create(pepper_drm_t *drm, const char *renderer);

void
pepper_drm_output_destroy(pepper_drm_t *drm);

#endif /* DRM_INTERNAL_H */
