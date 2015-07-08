#include <fcntl.h>
#include <pixman.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gbm.h>
#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "drm-internal.h"

#include <pepper-pixman-renderer.h>
#include <pepper-gl-renderer.h>

static pepper_bool_t
init_renderer(drm_output_t *output, const char *renderer);

static void
fini_renderer(drm_output_t *output);

void
drm_output_destroy(void *o)
{
    drm_output_t   *output = (drm_output_t *)o;

    wl_list_remove(&output->link);

    if (output->saved_crtc)
    {
        drmModeCrtc *c = output->saved_crtc;
        drmModeSetCrtc(output->drm->drm_fd, c->crtc_id, c->buffer_id, c->x, c->y,
                       &output->conn_id, 1, &c->mode);
        drmModeFreeCrtc(c);
    }

    fini_renderer(output);

    if (output->modes)
        free(output->modes);

    wl_signal_emit(&output->destroy_signal, NULL);

    free(output);
}

static void
drm_output_add_destroy_listener(void *o, struct wl_listener *listener)
{
    drm_output_t *output = (drm_output_t *)o;
    wl_signal_add(&output->destroy_signal, listener);
}

static void
drm_output_add_mode_change_listener(void *o, struct wl_listener *listener)
{
    drm_output_t *output = (drm_output_t *)o;
    wl_signal_add(&output->mode_change_signal, listener);
}

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
    drm_output_t       *output = (drm_output_t *)o;
    drmModeConnector   *c = drmModeGetConnector(output->drm->drm_fd, output->conn_id);
    int                 count = c->count_modes;

    drmModeFreeConnector(c);

    return count;
}

static void
drm_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    drm_output_t       *output = (drm_output_t *)o;
    drmModeModeInfo    *m = &(output->modes[index]);

    mode->flags = 0;
    mode->w = m->hdisplay;
    mode->h = m->vdisplay;
    mode->refresh = 60000/* FIXME */;

    if (m->type & DRM_MODE_TYPE_PREFERRED)
        mode->flags |= WL_OUTPUT_MODE_PREFERRED;

    if (m == output->current_mode)
        mode->flags |= WL_OUTPUT_MODE_CURRENT;
}

static pepper_bool_t
drm_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    int                 i;
    drm_output_t       *output = (drm_output_t *)o;

    /*
     *  typedef struct _drmModeModeInfo {
     *      uint32_t clock;
     *      uint16_t hdisplay, hsync_start, hsync_end, htotal, hskew;
     *      uint16_t vdisplay, vsync_start, vsync_end, vtotal, vscan;
     *
     *      uint32_t vrefresh;
     *
     *      uint32_t flags;
     *      uint32_t type;
     *      char name[DRM_DISPLAY_MODE_LEN];
     *  } drmModeModeInfo, *drmModeModeInfoPtr;
     */
    for (i = 0; i < output->mode_count; i++)
    {
        drmModeModeInfo *m = &(output->modes[i]);
        if ((m->hdisplay == mode->w) && (m->vdisplay == mode->h))   /* FIXME */
        {
            /* TODO: drmModeSetCrtc() with current front buffer with the mode. */

            output->current_mode = m;
            output->w = m->hdisplay;
            output->h = m->vdisplay;

            /* TODO: Resize handleing. */

            wl_signal_emit(&output->mode_change_signal, NULL);
            return PEPPER_TRUE;
        }
    }

    return PEPPER_FALSE;
}

static void
destroy_fb(struct gbm_bo *bo, void *data)
{
    drm_fb_t *fb = (drm_fb_t *)data;

    if (fb->id)
        drmModeRmFB(fb->fd, fb->id);

    free(fb);
}

static drm_fb_t *
get_fb(drm_output_t *output, struct gbm_bo *bo)
{
    drm_fb_t   *fb = (drm_fb_t *)gbm_bo_get_user_data(bo);

    if (fb)
        return fb;

    /* Create drm_fb for the new comer. */
    fb = (drm_fb_t *)calloc(1, sizeof(drm_fb_t));
    if (!fb)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    fb->fd = output->drm->drm_fd;
    fb->handle = gbm_bo_get_handle(bo).u32;

    fb->w = gbm_bo_get_width(bo);
    fb->h = gbm_bo_get_height(bo);
    fb->stride = gbm_bo_get_stride(bo);
    fb->size = fb->stride * fb->h;
    fb->bo = bo;

    if (drmModeAddFB(fb->fd, fb->w, fb->h, 24, 32, fb->stride, fb->handle, &fb->id))
    {
        PEPPER_ERROR("Failed to add fb in %s\n", __FUNCTION__);
        free(fb);
        return NULL;
    }

    gbm_bo_set_user_data(bo, fb, destroy_fb);
    return fb;
}

static void
update_back_buffer(drm_output_t *output)
{
    if (output->renderer == output->drm->gl_renderer)
    {
        struct gbm_bo *bo = gbm_surface_lock_front_buffer(output->gbm_surface);

        if (!bo)
        {
            PEPPER_ERROR("gbm_surface_lock_front_buffer() failed.\n");
            output->back_fb = NULL;
            return;
        }

        output->back_fb = get_fb(output, bo);
        if (!output->back_fb)
        {
            PEPPER_ERROR("Failed to get drm_fb from gbm_bo.\n");
            return;
        }
    }
    else if (output->renderer == output->drm->pixman_renderer)
    {
        output->back_fb = output->dumb_fb[output->back_fb_index];
    }
    else
    {
        output->back_fb = NULL;
    }
}

static void
drm_output_repaint(void *o, const pepper_list_t *view_list, const pixman_region32_t *damage)
{
    int             ret;
    drm_output_t   *output = (drm_output_t *)o;

    pepper_renderer_set_target(output->renderer, output->render_target);
    pepper_renderer_repaint_output(output->renderer, output->base, view_list, damage);

    update_back_buffer(output);

    if (!output->back_fb)
        return;

    if (!output->front_fb)
    {
        ret = drmModeSetCrtc(output->drm->drm_fd, output->crtc_id, output->back_fb->id,
                             0, 0, &output->conn_id, 1, output->current_mode);
    }

    ret = drmModePageFlip(output->drm->drm_fd, output->crtc_id, output->back_fb->id,
                          DRM_MODE_PAGE_FLIP_EVENT, output);
    if (ret < 0)
    {
        PEPPER_ERROR("Failed to queue pageflip in %s\n", __FUNCTION__);
        return;
    }

    output->page_flip_pending = PEPPER_TRUE;

    /* TODO: set planes */
}

static void
drm_output_attach_surface(void *o, pepper_object_t *surface, int *w, int *h)
{
    pepper_renderer_attach_surface(((drm_output_t *)o)->renderer, surface, w, h);
}

static void
drm_output_add_frame_listener(void *o, struct wl_listener *listener)
{
    drm_output_t *output = (drm_output_t *)o;
    wl_signal_add(&output->frame_signal, listener);
}

struct pepper_output_backend drm_output_backend =
{
    drm_output_destroy,
    drm_output_add_destroy_listener,
    drm_output_add_mode_change_listener,

    drm_output_get_subpixel_order,
    drm_output_get_maker_name,
    drm_output_get_model_name,

    drm_output_get_mode_count,
    drm_output_get_mode,
    drm_output_set_mode,

    drm_output_repaint,
    drm_output_attach_surface,
    drm_output_add_frame_listener,
};

static struct udev_device *
find_primary_gpu(struct udev *udev) /* FIXME: copied from weston */
{
    struct udev_enumerate   *e;
    struct udev_list_entry  *entry;
    struct udev_device      *pci, *device, *drm_device = NULL;
    const char              *path, *device_seat, *id;

    e = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(e, "drm");
    udev_enumerate_add_match_sysname(e, "card[0-9]*");
    udev_enumerate_scan_devices(e);

    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e))
    {
        path = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(udev, path);
        if (!device)
            continue;
        device_seat = udev_device_get_property_value(device, "ID_SEAT");
        if (!device_seat)
            device_seat = "seat0";
        if (strcmp(device_seat, "seat0" /* FIXME: default seat? */))
        {
            udev_device_unref(device);
            continue;
        }

        pci = udev_device_get_parent_with_subsystem_devtype(device, "pci", NULL);
        if (pci)
        {
            id = udev_device_get_sysattr_value(pci, "boot_vga");
            if (id && !strcmp(id, "1"))
            {
                if (drm_device)
                    udev_device_unref(drm_device);
                drm_device = device;
                break;
            }
        }

        if (!drm_device)
            drm_device = device;
        else
            udev_device_unref(device);
    }

    udev_enumerate_unref(e);
    return drm_device;
}

static int
drm_open(const char *path, int flags)
{
    int             fd;
    struct stat     s;
    drm_magic_t     m;

    fd = open(path, flags | O_CLOEXEC);
    if (fd == -1)
    {
        PEPPER_ERROR("Failed to open file[%s] in %s\n", path, __FUNCTION__);
        goto error;
    }

    if (fstat(fd, &s) == -1)
    {
        PEPPER_ERROR("Failed to get file[%s] state in %s\n", path, __FUNCTION__);
        goto error;
    }

    if (major(s.st_rdev) != 226/*drm major*/)
    {
        PEPPER_ERROR("File %s is not a drm device file\n", path);
        goto error;
    }

    if ((drmGetMagic(fd, &m) != 0) || (drmAuthMagic(fd, m) != 0))
    {
        PEPPER_ERROR("drm fd is not master\n");
        goto error;
    }

    return fd;

error:
    if (fd > 0)
        close(fd);
    return -1;
}

static int
find_crtc(pepper_drm_t *drm, drmModeRes *res, drmModeConnector *conn)
{
    int             i, j;
    drmModeEncoder *enc;
    drm_output_t   *output;

    for (i = 0; i < conn->count_encoders; i++)
    {
        enc = drmModeGetEncoder(drm->drm_fd, conn->encoders[i]);
        if (!enc)
            continue;

        for (j = 0; j < res->count_crtcs; j++)
        {
            pepper_bool_t crtc_used = PEPPER_FALSE;

            if (!(enc->possible_crtcs & (1 << j)))
                continue;

            wl_list_for_each(output, &drm->output_list, link)
            {
                if (res->crtcs[j] == output->crtc_id)
                {
                    crtc_used = PEPPER_TRUE;
                    break;
                }
            }

            if (!crtc_used)
            {
                drmModeFreeEncoder(enc);
                return res->crtcs[j];
            }
        }
        drmModeFreeEncoder(enc);
    }

    return -1;
}

static void
destroy_dumb_fb(drm_fb_t *fb)
{
    if (fb->map)
        munmap(fb->map, fb->size);

    if (fb->id)
        drmModeRmFB(fb->fd, fb->id);

    if (fb->handle)
    {
        struct drm_mode_destroy_dumb destroy_arg;

        memset(&destroy_arg, 0, sizeof(destroy_arg));
        destroy_arg.handle = fb->handle;
        drmIoctl(fb->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg);
    }

    if (fb->target)
        pepper_render_target_destroy(fb->target);

    free(fb);
}

static drm_fb_t *
create_dumb_fb(drm_output_t *output)
{
    drm_fb_t   *fb;

    int         drm_fd = output->drm->drm_fd;
    uint32_t    width = output->w;
    uint32_t    height = output->h;

    struct drm_mode_create_dumb     create_arg;
    struct drm_mode_map_dumb        map_arg;

    fb = calloc(1, sizeof(drm_fb_t));
    if (!fb)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return NULL;
    }

    fb->fd = drm_fd;

    memset(&create_arg, 0, sizeof create_arg);
    create_arg.bpp = 32;
    create_arg.width = width;
    create_arg.height = height;

    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg))
    {
        PEPPER_ERROR("Failed to create dumb_fb fb with ioctl in %s\n", __FUNCTION__);
        goto error;
    }

    fb->handle = create_arg.handle;
    fb->stride = create_arg.pitch;
    fb->size = create_arg.size;

    if (drmModeAddFB(drm_fd, width, height, 24, 32, fb->stride, fb->handle, &fb->id))
    {
        PEPPER_ERROR("Failed to add fb in %s\n", __FUNCTION__);
        goto error;
    }

    memset(&map_arg, 0, sizeof map_arg);
    map_arg.handle = fb->handle;

    if (drmIoctl(fb->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_arg))
    {
        PEPPER_ERROR("Failed to map dumb_fb fb in %s\n", __FUNCTION__);
        goto error;
    }

    fb->map = mmap(0, fb->size, PROT_WRITE, MAP_SHARED, drm_fd, map_arg.offset);
    if (fb->map == MAP_FAILED)
    {
        PEPPER_ERROR("Failed to map fb in %s\n", __FUNCTION__);
        goto error;
    }

    fb->target = pepper_pixman_renderer_create_target(PEPPER_FORMAT_XRGB8888, fb->map,
                                                      fb->stride, width, height);
    if (!fb->target)
    {
        PEPPER_ERROR("Failed to create pixman render target.\n");
        goto error;
    }

    return fb;

error:
    destroy_dumb_fb(fb);
    return NULL;
}

static void
fini_pixman_renderer(drm_output_t *output)
{
    int i;

    for (i = 0; i < DUMB_FB_COUNT; i++)
    {
        if (output->dumb_fb[i])
            destroy_dumb_fb(output->dumb_fb[i]);
    }
}

/* FIXME: copied from weston */
static pepper_bool_t
init_pixman_renderer(drm_output_t *output)
{
    int i;

    for (i = 0; i < DUMB_FB_COUNT; i++)
    {
        output->dumb_fb[i] = create_dumb_fb(output);
        if (!output->dumb_fb[i])
        {
            PEPPER_ERROR("Failed to create dumb_fb[%d] in %s\n", i, __FUNCTION__);
            return PEPPER_FALSE;
        }
    }

    output->renderer = output->drm->pixman_renderer;
    output->render_target = output->dumb_fb[output->back_fb_index]->target;
    output->use_pixman = PEPPER_TRUE;

    return PEPPER_TRUE;
}

static void
fini_gl_renderer(drm_output_t *output)
{
    if (output->gl_render_target)
        pepper_render_target_destroy(output->gl_render_target);

    if (output->gbm_surface)
        gbm_surface_destroy(output->gbm_surface);

    output->gl_render_target = NULL;
    output->gbm_surface = NULL;
}

static pepper_bool_t
init_gl_renderer(drm_output_t *output)
{
    uint32_t native_visual_id;

    if (!output->drm->gl_renderer)
        return PEPPER_FALSE;

    output->gbm_surface = gbm_surface_create(output->drm->gbm_device, output->w, output->h,
                                             GBM_FORMAT_XRGB8888/*FIXME*/,
                                             GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING/*FIXME*/);
    if (!output->gbm_surface)
    {
        PEPPER_ERROR("Failed to create gbm surface in %s\n", __FUNCTION__);
        goto error;
    }

    native_visual_id = GBM_FORMAT_XRGB8888;
    output->gl_render_target = pepper_gl_renderer_create_target(output->drm->gl_renderer,
                                                                output->gbm_surface,
                                                                PEPPER_FORMAT_XRGB8888,
                                                                &native_visual_id);
    if (!output->gl_render_target)
    {
        PEPPER_ERROR("Failed to create gl render target.\n");
        goto error;
    }

    output->renderer = output->drm->gl_renderer;
    output->render_target = output->gl_render_target;

    return PEPPER_TRUE;

error:
    fini_gl_renderer(output);
    return PEPPER_FALSE;
}

static pepper_bool_t
init_renderer(drm_output_t *output, const char *renderer)
{
    if (strcmp(renderer, "gl") == 0)
        return init_gl_renderer(output);

    return init_pixman_renderer(output);
}

static void
fini_renderer(drm_output_t *output)
{
    if (output->use_pixman)
        fini_pixman_renderer(output);
    else
        fini_gl_renderer(output);
}

static drm_output_t *
drm_output_create(pepper_drm_t *drm, struct udev_device *device,
                  drmModeRes *res, drmModeConnector *conn)
{
    int             i;
    drm_output_t   *output;

    output = (drm_output_t *)calloc(1, sizeof(drm_output_t));
    if (!output)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    output->drm = drm;
    output->subpixel = conn->subpixel;

    wl_signal_init(&output->destroy_signal);
    wl_signal_init(&output->mode_change_signal);
    wl_signal_init(&output->frame_signal);

    wl_list_insert(&drm->output_list, &output->link);

    /* find crtc + connector */
    output->crtc_id = find_crtc(drm, res, conn);
    if (output->crtc_id < 0)
    {
        PEPPER_ERROR("Failed to find crtc in %s\n", __FUNCTION__);
        goto error;
    }
    output->conn_id = conn->connector_id;
    output->saved_crtc = drmModeGetCrtc(drm->drm_fd, output->crtc_id);

    /* set modes */
    output->mode_count = conn->count_modes;
    output->modes = (drmModeModeInfo *)calloc(conn->count_modes,
                                                     sizeof(drmModeModeInfo));
    if (!output->modes)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    memcpy(output->modes, conn->modes, conn->count_modes * sizeof(drmModeModeInfo));
    for (i = 0; i < output->mode_count; i++)
    {
        drmModeModeInfo *m = &(output->modes[i]);
        if (m->type & DRM_MODE_TYPE_PREFERRED)
        {
            output->current_mode = m;
            output->w = m->hdisplay;
            output->h = m->vdisplay;
        }
    }

    if (!init_renderer(output, drm->renderer))
    {
        PEPPER_ERROR("Failed to initialize renderer in %s\n", __FUNCTION__);
        goto error;
    }

    return output;

error:

    if (output)
        drm_output_destroy(output);

    return NULL;
}

static pepper_bool_t
add_outputs(pepper_drm_t *drm, struct udev_device *device)
{
    int                 i;
    drmModeRes         *res;
    drmModeConnector   *conn;
    drm_output_t       *output;

    res = drmModeGetResources(drm->drm_fd);
    if (!res)
    {
        PEPPER_ERROR("Failed to get drm resources in %s\n", __FUNCTION__);
        return PEPPER_FALSE;
    }

    drm->crtcs = calloc(res->count_crtcs, sizeof(uint32_t));
    if (!drm->crtcs)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        return PEPPER_FALSE;
    }
    drm->count_crtcs = res->count_crtcs;
    memcpy(drm->crtcs, res->crtcs, sizeof(uint32_t) * res->count_crtcs);

    drm->min_width = res->min_width;
    drm->min_height = res->min_height;
    drm->max_width = res->max_width;
    drm->max_height = res->max_height;

    for (i = 0; i < res->count_connectors; i++)
    {
        conn = drmModeGetConnector(drm->drm_fd, res->connectors[i]);
        if (!conn)
            continue;

        if (conn->connection != DRM_MODE_CONNECTED/* CHECKME */)
        {
            drmModeFreeConnector(conn);
            continue;
        }

        /* TODO: Get renderer string from somewhere else. i.e. config file. */
        output = drm_output_create(drm, device, res, conn);
        if (!output)
        {
            PEPPER_ERROR("Failed to create drm_output in %s\n", __FUNCTION__);
            drmModeFreeConnector(conn);
            continue;
        }

        /*
         * PEPPER_API pepper_output_t *
         * pepper_compositor_add_output(pepper_compositor_t *compositor,
         *                              const pepper_output_backend_t *backend, void *data)
         */
        output->base = pepper_compositor_add_output(output->drm->compositor,
                                                    &drm_output_backend, output);
        if (!output->base)
        {
            PEPPER_ERROR("Failed to add output to compositor in %s\n", __FUNCTION__);
            drm_output_destroy(output);
            drmModeFreeConnector(conn);
            continue;
        }

        drmModeFreeConnector(conn);
    }

    if (wl_list_empty(&drm->output_list))
    {
        PEPPER_ERROR("Failed to find active output in %s\n", __FUNCTION__);
        drmModeFreeResources(res);
        return PEPPER_FALSE;
    }

    drmModeFreeResources(res);

    return PEPPER_TRUE;
}

static void
handle_vblank(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec,
              void *user_data)
{
    /* TODO */
}

static void
handle_page_flip(int fd, unsigned int sequence, unsigned int tv_sec, unsigned int tv_usec,
                 void *user_data)
{
    drm_output_t *output = (drm_output_t *)user_data;

    if (output->use_pixman)
    {
        output->back_fb_index ^= 1;
        output->render_target = output->dumb_fb[output->back_fb_index]->target;
    }
    else
    {
        /* Assume GL renderer in this case. */
        if (output->front_fb && output->front_fb->bo)
            gbm_surface_release_buffer(output->gbm_surface, output->front_fb->bo);
    }

    output->front_fb = output->back_fb;
    output->back_fb = NULL;
    output->page_flip_pending = PEPPER_FALSE;

    if (!output->vblank_pending)
    {
        wl_signal_emit(&output->frame_signal, NULL);
    }
}

static int
handle_drm_events(int fd, uint32_t mask, void *data)
{
    drmEventContext ctx;

    memset(&ctx, 0, sizeof(drmEventContext));
    ctx.version = DRM_EVENT_CONTEXT_VERSION;
    ctx.vblank_handler = handle_vblank;
    ctx.page_flip_handler = handle_page_flip;
    drmHandleEvent(fd, &ctx);

    return 0;
}

static void
update_outputs(pepper_drm_t *drm, struct udev_device *device)
{
    int                 i;
    drmModeRes         *res;
    drmModeConnector   *conn;
    drm_output_t       *output;

    res = drmModeGetResources(drm->drm_fd);
    if (!res)
    {
        PEPPER_ERROR("Failed to get drm resources in %s\n", __FUNCTION__);
        return;
    }

    for (i = 0; i < res->count_connectors; i++)
    {
        conn = drmModeGetConnector(drm->drm_fd, res->connectors[i]);
        if (!conn)
            continue;

        wl_list_for_each(output, &drm->output_list, link)
            if (output->conn_id == conn->connector_id)
                break;

        if (output && conn->connection != DRM_MODE_CONNECTED)
        {
            drm_output_destroy(output);
        }
        else if (!output && conn->connection == DRM_MODE_CONNECTED)
        {
            /* TODO: Get renderer string from somewhere else. */
            output = drm_output_create(drm, device, res, conn);
            if (!output)
            {
                PEPPER_ERROR("Failed to create drm_output in %s\n", __FUNCTION__);
                drmModeFreeConnector(conn);
                continue;
            }

            output->base = pepper_compositor_add_output(output->drm->compositor,
                                                        &drm_output_backend, output);
            if (!output->base)
            {
                PEPPER_ERROR("Failed to add output to compositor in %s\n", __FUNCTION__);
                drm_output_destroy(output);
                drmModeFreeConnector(conn);
                continue;
            }
        }

        drmModeFreeConnector(conn);
    }

    drmModeFreeResources(res);
}

static int
handle_udev_drm_events(int fd, uint32_t mask, void *data)
{
    pepper_drm_t *drm = (pepper_drm_t *)data;
    struct udev_device *device;

    const char *sysnum;
    const char *value;

    device = udev_monitor_receive_device(drm->udev_monitor);

    sysnum = udev_device_get_sysnum(device);
    if (!sysnum || atoi(sysnum) != drm->drm_sysnum)
        goto done;

    value = udev_device_get_property_value(device, "HOTPLUG");
    if (!value || strcmp(value, "1"))
        goto done;

    update_outputs(drm, device);

done:
    udev_device_unref(device);
    return 0;
}

pepper_bool_t
pepper_drm_output_create(pepper_drm_t *drm, const char *renderer)
{
    struct udev_device      *drm_device;
    const char              *filepath;
    const char              *sysnum;

    struct wl_display       *display;
    struct wl_event_loop    *loop;

    /* drm open */
    drm_device = find_primary_gpu(drm->udev);
    if (!drm_device)
    {
        PEPPER_ERROR("Failed to find primary gpu in %s\n", __FUNCTION__);
        goto error;
    }

    sysnum = udev_device_get_sysnum(drm_device);
    if (!sysnum)
    {
        PEPPER_ERROR("Failed to get sysnum in %s\n", __FUNCTION__);
        goto error;
    }

    drm->drm_sysnum = atoi(sysnum);
    if (drm->drm_sysnum < 0)
    {
        PEPPER_ERROR("Failed to get sysnum in %s\n", __FUNCTION__);
        goto error;
    }

    filepath = udev_device_get_devnode(drm_device);
    drm->drm_fd = drm_open(filepath, O_RDWR);
    if (drm->drm_fd < 0)
    {
        PEPPER_ERROR("Failed to open drm[%s] in %s\n", filepath, __FUNCTION__);
        goto error;
    }

    if (renderer)
        drm->renderer = strdup(renderer);

    /* create gl-renderer & pixman-renderer */
    drm->gbm_device = gbm_create_device(drm->drm_fd);
    if (drm->gbm_device)
        drm->gl_renderer = pepper_gl_renderer_create(drm->compositor, drm->gbm_device, "gbm");

    drm->pixman_renderer = pepper_pixman_renderer_create(drm->compositor);
    if (!drm->pixman_renderer)
    {
        PEPPER_ERROR("Failed to create pixman-renderer\n");
        goto error;
    }

    /* add outputs */
    if (add_outputs(drm, drm_device) == PEPPER_FALSE)
    {
        PEPPER_ERROR("Failed to add outputs in %s\n", __FUNCTION__);
        goto error;
    }

    /* add drm fd to main loop */
    display = pepper_compositor_get_display(drm->compositor);
    loop = wl_display_get_event_loop(display);
    drm->drm_event_source = wl_event_loop_add_fd(loop, drm->drm_fd, WL_EVENT_READABLE,
                                                 handle_drm_events, drm);
    if (!drm->drm_event_source)
    {
        PEPPER_ERROR("Failed to add drm fd to main loop in %s\n", __FUNCTION__);
        goto error;
    }

    drm->udev_monitor = udev_monitor_new_from_netlink(drm->udev, "udev");
    if (!drm->udev_monitor)
    {
        PEPPER_ERROR("Failed to create udev_monitor in %s\n", __FUNCTION__);
        goto error;
    }

    udev_monitor_filter_add_match_subsystem_devtype(drm->udev_monitor, "drm", NULL);
    drm->udev_monitor_source = wl_event_loop_add_fd(loop,
                                                    udev_monitor_get_fd(drm->udev_monitor),
                                                    WL_EVENT_READABLE,
                                                    handle_udev_drm_events, drm);
    if (!drm->udev_monitor_source)
    {
        PEPPER_ERROR("Failed to add udev_monitor fd to main loop in %s\n", __FUNCTION__);
        goto error;
    }

    if (udev_monitor_enable_receiving(drm->udev_monitor) < 0)
    {
        PEPPER_ERROR("Failed to enable udev_monitor in %s\n", __FUNCTION__);
        goto error;
    }


    udev_device_unref(drm_device);

    return PEPPER_TRUE;

error:

    if (drm_device)
        udev_device_unref(drm_device);

    return PEPPER_FALSE;
}

void
pepper_drm_output_destroy(drm_output_t *output)
{
    drm_output_destroy(output);
}
