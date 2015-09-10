#ifndef DRM_INTERNAL_H
#define DRM_INTERNAL_H

#include <pixman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#include <pepper-output-backend.h>
#include <pepper-libinput.h>
#include <pepper-render.h>

#include "pepper-drm.h"

typedef struct drm_output       drm_output_t;
typedef struct drm_buffer       drm_buffer_t;
typedef struct drm_plane        drm_plane_t;
typedef struct drm_connector    drm_connector_t;

typedef enum drm_buffer_type
{
    DRM_BUFFER_TYPE_DUMB,
    DRM_BUFFER_TYPE_GBM,
    DRM_BUFFER_TYPE_CLIENT,
} drm_buffer_type_t;

typedef enum drm_render_type
{
    DRM_RENDER_TYPE_PIXMAN,
    DRM_RENDER_TYPE_GL,
} drm_render_type_t;

struct pepper_drm
{
    pepper_compositor_t        *compositor;

    int                         fd;
    int                         sysnum;
    struct udev_monitor        *udev_monitor;

    drmEventContext             drm_event_context;
    struct wl_event_source     *drm_event_source;
    struct wl_event_source     *udev_event_source;

    pepper_list_t               connector_list;
    uint32_t                    used_crtcs;
    pepper_list_t               plane_list;

    drmModeRes                 *resources;
    struct gbm_device          *gbm_device;

    pepper_renderer_t          *pixman_renderer;
    pepper_renderer_t          *gl_renderer;
};

struct drm_connector
{
    pepper_drm_t       *drm;
    char                name[32];
    uint32_t            id;
    pepper_bool_t       connected;
    drm_output_t       *output;
    drmModeConnector   *connector;

    pepper_list_t       link;
};

void
drm_init_connectors(pepper_drm_t *drm);

void
drm_update_connectors(pepper_drm_t *drm);

void
drm_connector_destroy(drm_connector_t *conn);

struct drm_buffer
{
    pepper_drm_t            *drm;
    drm_buffer_type_t        type;
    uint32_t                 id;
    uint32_t                 handle;

    uint32_t                 w, h;
    uint32_t                 stride;
    uint32_t                 size;

    pepper_buffer_t         *client_buffer;
    pepper_event_listener_t *client_buffer_destroy_listener;
    struct gbm_surface      *surface;
    struct gbm_bo           *bo;
    void                    *map;
    pepper_render_target_t  *pixman_render_target;
};

drm_buffer_t *
drm_buffer_create_dumb(pepper_drm_t *drm, uint32_t w, uint32_t h);

drm_buffer_t *
drm_buffer_create_gbm(pepper_drm_t *drm, struct gbm_surface *surface, struct gbm_bo *bo);

drm_buffer_t *
drm_buffer_create_pepper(pepper_drm_t *drm, pepper_buffer_t *buffer);

void
drm_buffer_release(drm_buffer_t *buffer);

void
drm_buffer_destroy(drm_buffer_t *buffer);

struct drm_output
{
    pepper_drm_t           *drm;
    pepper_output_t        *base;

    drm_connector_t        *conn;
    int32_t                 crtc_index;
    uint32_t                crtc_id;
    drmModeCrtc            *saved_crtc;
    int32_t                 subpixel;
    drmModeModeInfo        *mode;

    pepper_bool_t           page_flip_pending;
    int                     vblank_pending_count;

    pepper_plane_t         *cursor_plane;
    pepper_plane_t         *primary_plane;
    pepper_plane_t         *fb_plane;

    drm_render_type_t       render_type;
    pepper_renderer_t      *renderer;
    pepper_render_target_t *render_target;

    /* pixman */
    drm_buffer_t           *fb[2];
    int                     back_fb_index;
    pixman_region32_t       previous_damage;

    /* OpenGL */
    struct gbm_surface     *gbm_surface;

    drm_buffer_t           *front, *back;
};

drm_output_t *
drm_output_create(drm_connector_t *conn);

void
drm_output_destroy(void *o);

void
drm_handle_vblank(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);

void
drm_handle_page_flip(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);

struct drm_plane
{
    pepper_drm_t   *drm;
    uint32_t        id;
    drmModePlane   *plane;

    drm_output_t   *output;
    pepper_plane_t *base;

    drm_buffer_t   *front, *back;
    int             dx, dy, dw, dh;
    int             sx, sy, sw, sh;

    pepper_list_t   link;
};

void
drm_init_planes(pepper_drm_t *drm);

void
drm_plane_destroy(drm_plane_t *plane);

#endif /* DRM_INTERNAL_H */
