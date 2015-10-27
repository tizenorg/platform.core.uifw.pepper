#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "drm-internal.h"
#include <pepper-pixman-renderer.h>
#include <pepper-gl-renderer.h>

static int32_t
drm_output_get_subpixel_order(void *data)
{
    drm_output_t *output = (drm_output_t *)data;

    switch (output->subpixel)
    {
        case DRM_MODE_SUBPIXEL_UNKNOWN:
            return WL_OUTPUT_SUBPIXEL_UNKNOWN;
        case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
            return WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;
        case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
            return WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR;
        case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
            return WL_OUTPUT_SUBPIXEL_VERTICAL_RGB;
        case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
            return WL_OUTPUT_SUBPIXEL_VERTICAL_BGR;
        case DRM_MODE_SUBPIXEL_NONE:
            return WL_OUTPUT_SUBPIXEL_NONE;
        default:
            return WL_OUTPUT_SUBPIXEL_UNKNOWN;
    }

    return WL_OUTPUT_SUBPIXEL_UNKNOWN;
}

static const char *
drm_output_get_maker_name(void *output)
{
    return "PePPer DRM";
}

static const char *
drm_output_get_model_name(void *output)
{
    return "PePPer DRM";
}

static int
drm_output_get_mode_count(void *o)
{
    drm_output_t *output = (drm_output_t *)o;
    return output->conn->connector->count_modes;
}

static uint32_t
get_refresh_rate(const drmModeModeInfo *info)
{
    return (info->clock * 1000000LL / info->vtotal + info->vtotal / 2) / info->vtotal;
}

static pepper_bool_t
same_mode(const drmModeModeInfo *info, const pepper_output_mode_t *mode)
{
    return info->hdisplay == mode->w && info->vdisplay == mode->h &&
           get_refresh_rate(info) == (uint32_t)mode->refresh;
}

static void
drm_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    drm_output_t    *output = (drm_output_t *)o;
    drmModeModeInfo *info = &output->conn->connector->modes[index];

    mode->flags = 0;
    mode->w = info->hdisplay;
    mode->h = info->vdisplay;
    mode->refresh = get_refresh_rate(info);

    if (info->type & DRM_MODE_TYPE_PREFERRED)
        mode->flags |= WL_OUTPUT_MODE_PREFERRED;

    if (info == output->mode)
        mode->flags |= WL_OUTPUT_MODE_CURRENT;
}

static pepper_bool_t
drm_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    int             i;
    drm_output_t   *output = (drm_output_t *)o;

    if (same_mode(output->mode, mode))
        return PEPPER_TRUE;

    for (i = 0; i < output->conn->connector->count_modes; i++)
    {
        drmModeModeInfo *info = &output->conn->connector->modes[i];

        if (same_mode(info, mode))
        {
            output->mode = info;
            pepper_output_update_mode(output->base);
            return PEPPER_TRUE;
        }
    }

    return PEPPER_FALSE;
}

static pepper_plane_t *
assign_cursor_plane(drm_output_t *output, pepper_view_t *view)
{
    /* TODO: */
    return NULL;
}

static pepper_plane_t *
assign_fb_plane(drm_output_t *output, pepper_view_t *view)
{
    double              x, y;
    int32_t             w, h, transform;
    pepper_surface_t   *surface;
    pepper_buffer_t    *buffer;

    const pepper_output_geometry_t *geometry;

    if (output->back)
        return NULL;

    if (!output->drm->gbm_device)
        return NULL;

    geometry = pepper_output_get_geometry(output->base);
    pepper_view_get_position(view, &x, &y);
    if ((geometry->x != (int)x) || (geometry->y != (int)y))
        return NULL;

    pepper_view_get_size(view, &w, &h);
    if ((geometry->w != w) || (geometry->h != h))
        return NULL;

    surface = pepper_view_get_surface(view);
    transform = pepper_surface_get_buffer_transform(surface);
    if (geometry->transform != transform)
        return NULL;

    buffer = pepper_surface_get_buffer(surface);
    if (!buffer)
        return NULL;

    output->back = drm_buffer_create_pepper(output->drm, buffer);
    PEPPER_CHECK(output->back, return NULL, "failed to create drm_buffer\n");

    return output->fb_plane;
}

static pepper_plane_t *
assign_overlay_plane(drm_output_t *output, pepper_view_t *view)
{
    drm_plane_t        *plane;
    pepper_surface_t   *surface;
    pepper_buffer_t    *buffer;
    struct wl_resource *resource;

    double              x, y;
    int                 w, h;

    if (!output->drm->gbm_device)
        return NULL;

    surface = pepper_view_get_surface(view);
    if (!surface)
        return NULL;

    buffer = pepper_surface_get_buffer(surface);
    if (!buffer)
        return NULL;

    resource = pepper_buffer_get_resource(buffer);
    if (!resource)
        return NULL;

    if (wl_shm_buffer_get(resource))
        return NULL;

    pepper_list_for_each(plane, &output->drm->plane_list, link)
    {
        if (!plane->back && (plane->plane->possible_crtcs & (1 << output->crtc_index)))
        {
            plane->back = drm_buffer_create_pepper(output->drm, buffer);
            PEPPER_CHECK(plane->back, return NULL, "failed to create drm_buffer\n");

            /* set position  */
            pepper_view_get_position(view, &x, &y);
            pepper_view_get_size(view, &w, &h);
            plane->dx = (int)x;
            plane->dy = (int)y;
            plane->dw = w;
            plane->dh = h;

            plane->sx = 0 << 16;
            plane->sy = 0 << 16;
            plane->sw = w << 16;
            plane->sh = h << 16;

            plane->output = output;

            return plane->base;
        }
    }

    return NULL;
}

static void
drm_output_assign_planes(void *o, const pepper_list_t *view_list)
{
    pepper_list_t      *l;
    drm_output_t       *output = o;

    pepper_list_for_each_list(l, view_list)
    {
        pepper_view_t      *view = l->item;
        pepper_plane_t     *plane = NULL;
        pepper_surface_t   *surface = pepper_view_get_surface(view);
        pepper_buffer_t    *buffer = pepper_surface_get_buffer(surface);

        if ((output->render_type == DRM_RENDER_TYPE_PIXMAN) ||
            (buffer &&
             (!wl_shm_buffer_get(pepper_buffer_get_resource(buffer)))))
            pepper_surface_set_keep_buffer(surface, PEPPER_TRUE);
        else
            pepper_surface_set_keep_buffer(surface, PEPPER_FALSE);

        if (plane == NULL)
            plane = assign_cursor_plane(output, view);

        if (plane == NULL)
            plane = assign_fb_plane(output, view);

        if (plane == NULL)
            plane = assign_overlay_plane(output, view);

        if (plane == NULL)
            plane = output->primary_plane;

        pepper_view_assign_plane(view, output->base, plane);
    }
}

static void
drm_output_render(drm_output_t *output)
{
    const pepper_list_t *render_list = pepper_plane_get_render_list(output->primary_plane);
    pixman_region32_t   *damage = pepper_plane_get_damage_region(output->primary_plane);
    pixman_region32_t    total_damage;

    if (output->render_type == DRM_RENDER_TYPE_PIXMAN)
    {
        pixman_region32_init(&total_damage);
        pixman_region32_union(&total_damage, damage, &output->previous_damage);
        pixman_region32_copy(&output->previous_damage, damage);
        damage = &total_damage;

        output->back_fb_index ^= 1;
        output->render_target = output->fb[output->back_fb_index]->pixman_render_target;
    }

    pepper_renderer_set_target(output->renderer, output->render_target);
    pepper_renderer_repaint_output(output->renderer, output->base, render_list, damage);
    pepper_plane_clear_damage_region(output->primary_plane);

    if (output->render_type == DRM_RENDER_TYPE_PIXMAN)
    {
        output->back = output->fb[output->back_fb_index];
        pixman_region32_fini(&total_damage);
    }
    else if (output->render_type == DRM_RENDER_TYPE_GL)
    {
        struct gbm_bo *bo = gbm_surface_lock_front_buffer(output->gbm_surface);
        PEPPER_CHECK(bo, return, "gbm_surface_lock_front_buffer() failed.\n");

        output->back = gbm_bo_get_user_data(bo);

        if (!output->back)
            output->back = drm_buffer_create_gbm(output->drm, output->gbm_surface, bo);
    }
}

static void
drm_output_repaint(void *o, const pepper_list_t *plane_list)
{
    drm_output_t   *output = o;
    drm_plane_t    *plane;
    int             ret;

    if (!output->back)
        drm_output_render(output);

    if (output->back)
    {
        if (!output->front)
        {
            ret = drmModeSetCrtc(output->drm->fd, output->crtc_id, output->back->id, 0, 0,
                                 &output->conn->id, 1, output->mode);
        }

        ret = drmModePageFlip(output->drm->fd, output->crtc_id, output->back->id,
                              DRM_MODE_PAGE_FLIP_EVENT, output);
        PEPPER_CHECK(ret == 0, , "page flip failed.\n");

        output->page_flip_pending = PEPPER_TRUE;
    }

    pepper_list_for_each(plane, &output->drm->plane_list, link)
    {
        drmVBlank vbl;

        if (plane->output != output || plane->back == NULL)
            continue;

        vbl.request.type = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT;

        ret = drmModeSetPlane(output->drm->fd, plane->id,
                              output->crtc_id, plane->back->id, 0,
                              plane->dx, plane->dy, plane->dw, plane->dh,
                              plane->sx, plane->sy, plane->sw, plane->sh);

        PEPPER_CHECK(ret == 0, continue, "drmModeSetPlane() failed.\n");
        pepper_plane_clear_damage_region(plane->base);

        if (output->crtc_index > 1)
        {
            vbl.request.type |= (output->crtc_index << DRM_VBLANK_HIGH_CRTC_SHIFT) &
                DRM_VBLANK_HIGH_CRTC_MASK;
        }
        else if (output->crtc_index > 0)
        {
            vbl.request.type |= DRM_VBLANK_SECONDARY;
        }

        vbl.request.sequence = 1;
        vbl.request.signal = (unsigned long)plane;

        ret = drmWaitVBlank(output->drm->fd, &vbl);
        PEPPER_CHECK(ret == 0, continue, "drmWaitVBlank() failed.\n");

        output->vblank_pending_count++;
    }
}

static void
drm_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
    drm_output_t *output = o;
    pepper_renderer_attach_surface(output->renderer, surface, w, h);
}

static void
drm_output_flush_surface_damage(void *o, pepper_surface_t *surface)
{
    pepper_renderer_flush_surface_damage(((drm_output_t *)o)->renderer, surface);
}

struct pepper_output_backend drm_output_backend =
{
    drm_output_destroy,

    drm_output_get_subpixel_order,
    drm_output_get_maker_name,
    drm_output_get_model_name,

    drm_output_get_mode_count,
    drm_output_get_mode,
    drm_output_set_mode,

    drm_output_assign_planes,
    drm_output_repaint,
    drm_output_attach_surface,
    drm_output_flush_surface_damage,
};

static int
find_crtc_for_connector(drm_connector_t *conn)
{
    int i, j;

    for (i = 0; i < conn->connector->count_encoders; i++)
    {
        int32_t         possible_crtcs;
        drmModeEncoder *encoder = drmModeGetEncoder(conn->drm->fd, conn->connector->encoders[i]);

        PEPPER_CHECK(encoder, continue, "drmModeGetEncoder() failed.\n");

        possible_crtcs = encoder->possible_crtcs;
        drmModeFreeEncoder(encoder);

        for (j = 0; j < conn->drm->resources->count_crtcs; j++)
        {
            if (!(possible_crtcs & (1 << j)))
                continue;

            if (!(conn->drm->used_crtcs & (1 << j)))
                return j;
        }
    }

    return -1;
}

static void
fini_pixman_renderer(drm_output_t *output)
{
    int i;

    for (i = 0; i < 2; i++)
    {
        if (output->fb[i])
        {
            drm_buffer_destroy(output->fb[i]);
            output->fb[i] = NULL;
        }
    }

    pixman_region32_fini(&output->previous_damage);
    output->renderer = NULL;
    output->render_target = NULL;
}

static void
init_pixman_renderer(drm_output_t *output)
{
    pepper_drm_t   *drm = output->drm;
    int             i;
    int             w = output->mode->hdisplay;
    int             h = output->mode->vdisplay;

    if (!drm->pixman_renderer)
    {
        drm->pixman_renderer = pepper_pixman_renderer_create(drm->compositor);
        PEPPER_CHECK(drm->pixman_renderer, return, "pepper_pixman_renderer_create() failed.\n");
    }

    output->renderer = drm->pixman_renderer;

    for (i = 0; i < 2; i++)
    {
        output->fb[i] = drm_buffer_create_dumb(drm, w, h);
        PEPPER_CHECK(output->fb[i], goto error, "drm_buffer_create_dumb() failed.\n");
    }

    pixman_region32_init(&output->previous_damage);
    output->render_type = DRM_RENDER_TYPE_PIXMAN;

    return;

error:
    fini_pixman_renderer(output);
}

static void
fini_gl_renderer(drm_output_t *output)
{
    if (output->render_target)
        pepper_render_target_destroy(output->render_target);

    if (output->gbm_surface)
        gbm_surface_destroy(output->gbm_surface);

    output->renderer = NULL;
    output->render_target = NULL;
    output->gbm_surface = NULL;
}

static void
init_gl_renderer(drm_output_t *output)
{
    pepper_drm_t    *drm = output->drm;
    int             w = output->mode->hdisplay;
    int             h = output->mode->vdisplay;
    uint32_t        native_visual_id = GBM_FORMAT_XRGB8888;

    if (!drm->gl_renderer)
    {
        if (!drm->gbm_device)
        {
            drm->gbm_device = gbm_create_device(drm->fd);
            PEPPER_CHECK(drm->gbm_device, return, "gbm_create_device() failed.\n");
        }

        drm->gl_renderer = pepper_gl_renderer_create(drm->compositor, drm->gbm_device, "gbm");
        PEPPER_CHECK(drm->gl_renderer, return, "pepper_gl_renderer_create() failed.\n");
    }

    output->renderer = drm->gl_renderer;

    output->gbm_surface = gbm_surface_create(drm->gbm_device, w, h, GBM_FORMAT_XRGB8888,
                                             GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    PEPPER_CHECK(output->gbm_surface, goto error, "gbm_surface_create() failed.\n");

    output->render_target = pepper_gl_renderer_create_target(drm->gl_renderer,
                                                             output->gbm_surface,
                                                             PEPPER_FORMAT_XRGB8888,
                                                             &native_visual_id, w, h);
    PEPPER_CHECK(output->render_target, goto error, "pepper_gl_renderer_create_target() failed.\n");
    output->render_type = DRM_RENDER_TYPE_GL;

    return;

error:
    fini_gl_renderer(output);
}

drm_output_t *
drm_output_create(drm_connector_t *conn)
{
    pepper_drm_t   *drm = conn->drm;
    drm_output_t   *output;
    drm_plane_t    *plane, *tmp;
    const char     *render_env = getenv("PEPPER_DRM_RENDERER");

    PEPPER_CHECK(conn->output == NULL, return NULL, "The connector already has an output.\n");

    output = calloc(1, sizeof(drm_output_t));
    PEPPER_CHECK(output, return NULL, "calloc() failed.\n");

    output->drm = drm;
    output->conn = conn;
    output->crtc_index = find_crtc_for_connector(conn);
    output->crtc_id = drm->resources->crtcs[output->crtc_index];
    output->saved_crtc = drmModeGetCrtc(drm->fd, output->crtc_id);
    output->mode = &conn->connector->modes[0];

    output->base = pepper_compositor_add_output(drm->compositor, &drm_output_backend,
                                                conn->name, output);
    PEPPER_CHECK(output->base, goto error, "pepper_compositor_add_output() failed.\n");

    if (render_env && strcmp(render_env, "gl") == 0)
        init_gl_renderer(output);

    if (!output->renderer)
    {
        /* Pixman is default. */
        init_pixman_renderer(output);
        PEPPER_CHECK(output->renderer, goto error, "Failed to initialize renderer.\n");
    }

    output->primary_plane = pepper_output_add_plane(output->base, NULL);
    PEPPER_CHECK(output->primary_plane, goto error, "pepper_output_add_plane() failed.\n");

    output->cursor_plane = pepper_output_add_plane(output->base, output->primary_plane);
    PEPPER_CHECK(output->cursor_plane, goto error, "pepper_output_add_plane() failed.\n");

    output->fb_plane = pepper_output_add_plane(output->base, output->primary_plane);
    PEPPER_CHECK(output->fb_plane, goto error, "pepper_output_add_plane() failed.\n");

    pepper_list_for_each_safe(plane, tmp, &output->drm->plane_list, link)
    {
        if (plane->output == NULL && (plane->plane->possible_crtcs & (1 << output->crtc_index)))
        {
            plane->base = pepper_output_add_plane(output->base, output->primary_plane);

            if (plane->base)
                plane->output = output;
        }
    }

    drm->used_crtcs |= (1 << output->crtc_index);
    conn->output = output;

    return output;

error:
    if (output->saved_crtc)
    {
        drmModeFreeCrtc(output->saved_crtc);
        output->saved_crtc = NULL;
    }

    if (output->base)
        pepper_output_destroy(output->base);

    return NULL;
}

void
drm_output_destroy(void *o)
{
    drm_output_t *output = o;
    drm_plane_t  *plane;

    if (output->render_type == DRM_RENDER_TYPE_PIXMAN)
        fini_pixman_renderer(output);
    else if (output->render_type == DRM_RENDER_TYPE_GL)
        fini_gl_renderer(output);

    if (output->saved_crtc)
    {
        drmModeSetCrtc(output->conn->drm->fd,
                       output->saved_crtc->crtc_id,
                       output->saved_crtc->buffer_id,
                       output->saved_crtc->x, output->saved_crtc->y,
                       &output->conn->connector->connector_id, 1, &output->saved_crtc->mode);
        drmModeFreeCrtc(output->saved_crtc);
    }

    if (output->fb_plane)
        pepper_plane_destroy(output->fb_plane);

    if (output->primary_plane)
        pepper_plane_destroy(output->primary_plane);

    /* Release all planes. */
    pepper_list_for_each(plane, &output->drm->plane_list, link)
    {
        if (plane->output == output)
        {
            plane->output = NULL;
            pepper_plane_destroy(plane->base);
        }
    }

    /* destroy renderer. */
    free(output);
}

void
drm_handle_vblank(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    drm_plane_t        *plane = data;
    struct timespec     ts;

    plane->output->vblank_pending_count--;

    if (plane->front)
        drm_buffer_release(plane->front);

    plane->front = plane->back;
    plane->back = NULL;

    if (plane->output->vblank_pending_count == 0 && !plane->output->page_flip_pending)
    {
        ts.tv_sec = sec;
        ts.tv_nsec = usec * 1000;
        pepper_output_finish_frame(plane->output->base, &ts);
    }
}

void
drm_handle_page_flip(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    drm_output_t       *output = data;
    struct timespec     ts;

    output->page_flip_pending = PEPPER_FALSE;

    if (output->front)
        drm_buffer_release(output->front);

    output->front = output->back;
    output->back = NULL;

    if (output->vblank_pending_count == 0)
    {
        ts.tv_sec = sec;
        ts.tv_nsec = usec * 1000;
        pepper_output_finish_frame(output->base, &ts);
    }
}
