/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include "x11-internal.h"

#include "pepper-gl-renderer.h"
#include "pepper-pixman-renderer.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>

static uint8_t
xcb_depth_get(xcb_screen_t *screen, xcb_visualid_t id)
{
    xcb_depth_iterator_t i;
    xcb_visualtype_iterator_t j;

    i = xcb_screen_allowed_depths_iterator(screen);
    for (; i.rem; xcb_depth_next(&i))
    {
        j = xcb_depth_visuals_iterator(i.data);
        for (; j.rem; xcb_visualtype_next(&j))
        {
            if (j.data->visual_id == id)
                return i.data->depth;
        }
    }
    return 0;
}

static xcb_visualtype_t *
xcb_visualtype_get(xcb_screen_t *screen, xcb_visualid_t id)
{
    if (screen)
    {
        xcb_depth_iterator_t depth_iter;

        depth_iter = xcb_screen_allowed_depths_iterator(screen);
        for (; depth_iter.rem; xcb_depth_next(&depth_iter))
        {
            xcb_visualtype_iterator_t visual_iter;

            visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            for (; visual_iter.rem; xcb_visualtype_next(&visual_iter))
                if (visual_iter.data->visual_id == id)
                    return visual_iter.data;
        }
    }
    return NULL;
}

static void
x11_output_wait_for_map(x11_output_t *output)
{
    xcb_generic_event_t     *event;
    xcb_map_notify_event_t  *map_notify;

    uint8_t response_type;
    int     mapped = 0, configured = 0;

    while (!mapped || !configured)
    {
        event = xcb_wait_for_event(output->connection->xcb_connection);
        response_type = event->response_type & ~0x80;

        switch (response_type)
        {
        case XCB_MAP_NOTIFY:
            map_notify = (xcb_map_notify_event_t *) event;
            if (map_notify->window == output->window)
                mapped = 1;
            break;
        case XCB_CONFIGURE_NOTIFY:
            configured = 1;
            break;
        }
    }
}

static int
frame_done_handler(void* data)
{
    x11_output_t *output = data;
    pepper_output_finish_frame(output->base, NULL);
    return 1;
}

static pepper_bool_t
gl_renderer_init(x11_output_t *output)
{
    output->gl_target = pepper_gl_renderer_create_target(output->connection->gl_renderer,
                                                         (void *)(intptr_t)output->window,
                                                         PEPPER_FORMAT_ARGB8888,
                                                         &output->connection->screen->root_visual,
                                                         output->w, output->h);

    if (!output->gl_target)
    {
        PEPPER_ERROR("Failed to create gl render target.\n");
        return PEPPER_FALSE;
    }

    output->renderer = output->connection->gl_renderer;
    output->target = output->gl_target;
    return PEPPER_TRUE;
}

static void
x11_shm_image_deinit(xcb_connection_t *conn, x11_shm_image_t *shm)
{
    if (shm->target)
    {
        pepper_render_target_destroy(shm->target);
        shm->target = NULL;
    }

    if (shm->segment)
    {
        xcb_shm_detach(conn, shm->segment);
        shm->segment = 0;
    }

    if (shm->buf)
    {
        shmdt(shm->buf);
        shm->buf = NULL;
    }

    if (shm->shm_id)
    {
        shmctl(shm->shm_id, IPC_RMID, NULL);
        shm->shm_id = -1;
    }
}

static pepper_bool_t
x11_shm_image_init(x11_shm_image_t *shm, xcb_connection_t *conn, int w, int h, int bpp)
{
    xcb_generic_error_t     *err = NULL;
    xcb_void_cookie_t        cookie;
    pepper_format_t          format;

    /* FIXME: Hard coded */
    if (bpp == 32)
    {
        format = PEPPER_FORMAT_ARGB8888;
    }
    else if (bpp == 16)
    {
        format = PEPPER_FORMAT_RGB565;
    }
    else
    {
        PEPPER_ERROR("cannot find pixman format\n");
        goto err;
    }

    /* Create MIT-SHM id and attach */
    shm->shm_id = shmget(IPC_PRIVATE, w * h * (bpp/ 8), IPC_CREAT | S_IRWXU);
    if (shm->shm_id == -1)
    {
        PEPPER_ERROR("shmget() failed\n");
        goto err;
    }

    shm->buf = shmat(shm->shm_id, NULL, 0 /* read/write */);
    if (-1 == (long)shm->buf)
    {
        PEPPER_ERROR("shmat() failed\n");
        goto err;
    }

    /* Create XCB-SHM segment and attach */
    shm->segment = xcb_generate_id(conn);
    cookie = xcb_shm_attach_checked(conn, shm->segment, shm->shm_id, 1);
    err = xcb_request_check(conn, cookie);
    if (err)
    {
        PEPPER_ERROR("xcb_shm_attach error %d\n", err->error_code);
        goto err;
    }

    shmctl(shm->shm_id, IPC_RMID, NULL);

    /* Now create pepper render target */
    shm->target = pepper_pixman_renderer_create_target(format, shm->buf, w * (bpp / 8), w, h);
    if (!shm->target)
    {
        PEPPER_ERROR("Failed to create pixman render target\n");
        goto err;
    }

    shm->format = format;
    shm->stride = w * (bpp / 8);
    shm->w = w;
    shm->h = h;

    return PEPPER_TRUE;

err:
    if (err)
        free(err);

    if (shm->buf)
	shmdt(shm->buf);

    if (shm->shm_id)
        shmctl(shm->shm_id, IPC_RMID, NULL);

    return PEPPER_FALSE;
}

static pepper_bool_t
x11_shm_init(x11_output_t *output)
{
    xcb_screen_iterator_t    scr_iter;
    xcb_format_iterator_t    fmt_iter;
    xcb_visualtype_t        *visual_type;
    xcb_connection_t        *xcb_conn = output->connection->xcb_connection;
    int bpp = 0;

    /* Check if XCB-SHM is available */
    {
        const xcb_query_extension_reply_t   *ext;
        ext = xcb_get_extension_data(xcb_conn, &xcb_shm_id);
        if (ext == NULL || !ext->present)
        {
            PEPPER_ERROR("xcb-shm extension is not available\n");
            return PEPPER_FALSE;
        }
    }

    /* Find root visual */
    scr_iter = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));
    visual_type = xcb_visualtype_get(scr_iter.data,
                                     scr_iter.data->root_visual);
    if (!visual_type)
    {
        PEPPER_ERROR("Failed to lookup visual for root window\n");
        return PEPPER_FALSE;;
    }

    output->depth = xcb_depth_get(scr_iter.data,
                                  scr_iter.data->root_visual);

    fmt_iter = xcb_setup_pixmap_formats_iterator(xcb_get_setup(xcb_conn));
    for (; fmt_iter.rem; xcb_format_next(&fmt_iter))
    {
        if (fmt_iter.data->depth == output->depth)
        {
            bpp = fmt_iter.data->bits_per_pixel;
            break;
        }
    }
    output->bpp = bpp;

    /* Init x11_shm_image */
    if (!x11_shm_image_init(&output->shm, xcb_conn, output->w, output->h, bpp))
    {
        PEPPER_ERROR("x11_shm_image_init failed\n");
        return PEPPER_FALSE;
    }

    output->gc = xcb_generate_id(xcb_conn);
    xcb_create_gc(xcb_conn, output->gc, output->window, 0, NULL);

    return PEPPER_TRUE;
}

static pepper_bool_t
pixman_renderer_init(x11_output_t *output)
{
    /* Initialize xcb-shm infra and init shm-buffer */
    if (!x11_shm_init(output))
    {
        PEPPER_ERROR("shm_init failed\n");
        return PEPPER_FALSE;
    }

    output->renderer = output->connection->pixman_renderer;
    output->target = output->shm.target;

    return PEPPER_TRUE;
}

static pepper_bool_t
renderer_init(x11_output_t *output, const char *renderer)
{
    if (!strcmp(renderer, "gl"))
    {
        if (gl_renderer_init(output))
            return PEPPER_TRUE;
    }

    /* Pixman is default renderer */
    return pixman_renderer_init(output);
}

void
x11_output_destroy(void *o)
{
    x11_output_t            *output = o;
    pepper_x11_connection_t *conn = output->connection;

    if (output->gl_target)
        pepper_render_target_destroy(output->gl_target);

    /* XXX */
    x11_shm_image_deinit(conn->xcb_connection, &output->shm);
    wl_event_source_remove(output->frame_done_timer);
    xcb_destroy_window(conn->xcb_connection, output->window);
    pepper_list_remove(&output->link);
    xcb_flush(conn->xcb_connection);
    free(output);
}

static int32_t
x11_output_get_subpixel_order(void *o)
{
    x11_output_t *output = o;
    return output->subpixel;
}

static const char *
x11_output_get_maker_name(void *o)
{
    return "PePPer_x11";
}

static const char *
x11_output_get_model_name(void *o)
{
    return "PePPer_x11";
}

static int
x11_output_get_mode_count(void *o)
{
    /* There's only one available mode in x11 backend which is also the current mode. */
    return 1;
}

static void
x11_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    x11_output_t *output = o;

    if (index != 0)
        return;

    mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    mode->w = output->w;
    mode->h = output->h;
    mode->refresh = 60000;
}

static pepper_bool_t
x11_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    x11_output_t *output = o;

    if (mode->w <= 0 || mode->h <= 0)
        return PEPPER_FALSE;

    if (mode->refresh != 60000)
        return PEPPER_FALSE;

    if (output->w != mode->w || output->h != mode->h)
    {
        output->w = mode->w;
        output->h = mode->h;

        /* Resize output window. */
        {
            xcb_connection_t *conn = output->connection->xcb_connection;
            xcb_size_hints_t hints;
            uint32_t values[2];

            values[0] = output->w;
            values[1] = output->h;

            /* set hints for window */
            memset(&hints, 0, sizeof(hints));
            hints.flags = WM_NORMAL_HINTS_MAX_SIZE | WM_NORMAL_HINTS_MIN_SIZE;
            hints.min_width  = hints.max_width  = output->w;
            hints.min_height = hints.max_height = output->h;
            xcb_change_property(conn,
                                XCB_PROP_MODE_REPLACE,
                                output->window,
                                output->connection->atom.wm_normal_hints,
                                output->connection->atom.wm_size_hints,
                                32,
                                sizeof(hints) / 4,
                                (uint8_t *)&hints);

            /* resize window */
            xcb_configure_window (conn,
                                  output->window,
                                  XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                  values);

            /* resize image */
            if (output->shm.target)
            {
                /* Release existing shm-buffer */
                x11_shm_image_deinit(conn, &output->shm);

                /* Init x11_shm_image */
                if (!x11_shm_image_init(&output->shm, conn, output->w, output->h, output->bpp))
                {
                    PEPPER_ERROR("x11_shm_image_init failed\n");
                    return PEPPER_FALSE;
                }
            }

            /* flush connection */
            xcb_flush(output->connection->xcb_connection);
        }

        pepper_output_update_mode(output->base);
    }

    return PEPPER_TRUE;
}

static void
x11_output_assign_planes(void *o, const pepper_list_t *view_list)
{
    x11_output_t   *output = (x11_output_t *)o;
    pepper_list_t  *l;

    pepper_list_for_each_list(l, view_list)
    {
        pepper_view_t *view = l->item;
        pepper_view_assign_plane(view, output->base, output->primary_plane);
    }
}

static void
x11_output_start_repaint_loop(void *o)
{
    x11_output_t    *output = o;
    struct timespec  ts;

    pepper_compositor_get_time(output->connection->compositor, &ts);
    pepper_output_finish_frame(output->base, &ts);
}

static void
x11_output_repaint(void *o, const pepper_list_t *plane_list)
{
    x11_output_t *output = o;
    pepper_list_t  *l;

    pepper_list_for_each_list(l, plane_list)
    {
        pepper_plane_t *plane = l->item;

        if (plane == output->primary_plane)
        {
            const pepper_list_t *render_list = pepper_plane_get_render_list(plane);
            pixman_region32_t   *damage = pepper_plane_get_damage_region(plane);

            pepper_renderer_set_target(output->renderer, output->target);
            pepper_renderer_repaint_output(output->renderer, output->base, render_list, damage);
            pepper_plane_clear_damage_region(plane);

            if (output->renderer == output->connection->pixman_renderer)
            {
                xcb_void_cookie_t    cookie;
                xcb_generic_error_t *err;

                cookie = xcb_shm_put_image_checked(output->connection->xcb_connection,
                                                   output->window,
                                                   output->gc,
                                                   output->w,
                                                   output->h,
                                                   0,   /* src_x */
                                                   0,   /* src_y */
                                                   output->shm.w,  /* src_w */
                                                   output->shm.h, /* src_h */
                                                   0,   /* dst_x */
                                                   0,   /* dst_y */
                                                   output->depth,   /* depth */
                                                   XCB_IMAGE_FORMAT_Z_PIXMAP,   /* format */
                                                   0,   /* send_event */
                                                   output->shm.segment, /* xcb shm segment */
                                                   0);  /* offset */

                err = xcb_request_check(output->connection->xcb_connection, cookie);
                if (err)
                {
                    PEPPER_ERROR("Failed to put shm image, err: %d\n", err->error_code);
                    free(err);
                }
            }

            /* XXX: frame_done callback called after 10ms, referenced from weston */
            wl_event_source_timer_update(output->frame_done_timer, 10);
        }

        /* TODO: Cursor??? */
    }
}

static void
x11_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
    pepper_renderer_attach_surface(((x11_output_t *)o)->renderer, surface, w, h);
}

static void
x11_output_flush_surface_damage(void *o, pepper_surface_t *surface, pepper_bool_t *keep_buffer)
{
    x11_output_t    *output = o;
    pepper_buffer_t *buffer = pepper_surface_get_buffer(surface);

    pepper_renderer_flush_surface_damage(output->renderer, surface);

    if (output->renderer == output->connection->pixman_renderer ||
        (buffer && !wl_shm_buffer_get(pepper_buffer_get_resource(buffer))))
    {
        *keep_buffer = PEPPER_TRUE;
    }
    else
    {
        *keep_buffer = PEPPER_FALSE;
    }
}

/* X11 output backend to export for PePPer core */
static const pepper_output_backend_t x11_output_backend =
{
    x11_output_destroy,

    x11_output_get_subpixel_order,
    x11_output_get_maker_name,
    x11_output_get_model_name,

    x11_output_get_mode_count,
    x11_output_get_mode,
    x11_output_set_mode,

    x11_output_assign_planes,
    x11_output_start_repaint_loop,
    x11_output_repaint,
    x11_output_attach_surface,
    x11_output_flush_surface_damage,
};

PEPPER_API pepper_output_t *
pepper_x11_output_create(pepper_x11_connection_t *connection,
                         int x, int y, int w, int h, int transform, int scale,
                         const char *renderer)
{
    static const char       *window_name = "PePPer Compositor";
    static const char       *class_name  = "pepper-1\0PePPer Compositor";

    pepper_output_t         *base;
    x11_output_t            *output;

    struct wl_display       *wldisplay;
    struct wl_event_loop    *loop;

    output = calloc(1, sizeof(x11_output_t));
    if (!output)
    {
        PEPPER_ERROR("memory allocation failed");
        return NULL;
    }

    output->connection = connection;
    output->w = w;
    output->h = h;

    /* Hard-Coded: subpixel order to horizontal RGB. */
    output->subpixel = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;

    /* Create X11 window */
    {
        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t values[2] = {
                connection->screen->white_pixel,
                XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        };
        xcb_size_hints_t hints;
        xcb_atom_t list[1];

        output->window = xcb_generate_id(connection->xcb_connection);
        xcb_create_window(connection->xcb_connection,
                          XCB_COPY_FROM_PARENT,
                          output->window,
                          connection->screen->root,
                          0, 0, w, h, 0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          connection->screen->root_visual,
                          mask,
                          values);

        /* cannot resize */
        memset(&hints, 0, sizeof(hints));
        hints.flags = WM_NORMAL_HINTS_MAX_SIZE | WM_NORMAL_HINTS_MIN_SIZE;
        hints.min_width  = hints.max_width  = w;
        hints.min_height = hints.max_height = h;
        xcb_change_property(connection->xcb_connection,
                            XCB_PROP_MODE_REPLACE,
                            output->window,
                            connection->atom.wm_normal_hints,
                            connection->atom.wm_size_hints,
                            32,
                            sizeof(hints) / 4,
                            (uint8_t *)&hints);

        /* set window name */
        xcb_change_property(connection->xcb_connection, XCB_PROP_MODE_REPLACE,
                            output->window,
                            connection->atom.net_wm_name,
                            connection->atom.utf8_string, 8,
                            strlen(window_name), window_name);
        xcb_change_property(connection->xcb_connection, XCB_PROP_MODE_REPLACE,
                            output->window,
                            connection->atom.wm_class,
                            connection->atom.string, 8,
                            strlen(class_name), class_name);

        /* set property to receive wm_delete_window message */
        list[0] = connection->atom.wm_delete_window;
	xcb_change_property(connection->xcb_connection, XCB_PROP_MODE_REPLACE,
			    output->window,
			    connection->atom.wm_protocols,
			    XCB_ATOM_ATOM, 32,
			    1, list);


        xcb_map_window(connection->xcb_connection, output->window);

        if (connection->use_xinput)
            x11_window_input_property_change(connection->xcb_connection,
                                             output->window);

        pepper_list_insert(&connection->output_list, &output->link);
        xcb_flush(connection->xcb_connection);
        x11_output_wait_for_map(output);
    }

    wldisplay = pepper_compositor_get_display(connection->compositor);
    loop = wl_display_get_event_loop(wldisplay);
    output->frame_done_timer = wl_event_loop_add_timer(loop, frame_done_handler, output);

    /* Init renderer */
    renderer_init(output, renderer);

    /* Register output object */
    snprintf(&output->name[0], 32, "x11-%p", output);
    base = pepper_compositor_add_output(connection->compositor,
                                        &x11_output_backend, output->name, output,
                                        transform, scale);
    if (!base)
    {
        PEPPER_ERROR("pepper_compositor_add_output failed\n");
        x11_output_destroy(output);
        return NULL;
    }

    output->base = base;
    output->primary_plane = pepper_output_add_plane(output->base, NULL);
    pepper_output_move(base, x, y);

    return base;
}
