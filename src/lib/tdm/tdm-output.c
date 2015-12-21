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

#include "tdm-internal.h"

#include <pepper-pixman-renderer.h>
#include <pepper-gl-renderer.h>

static int key_render_target;
static int key_tdm_buffer;

#define KEY_RENDER_TARGET ((unsigned long)&key_render_target)
#define KEY_TDM_BUFFER ((unsigned long)&key_tdm_buffer)

static void
__tdm_renderer_pixman_free_target(void *user_data)
{
    pepper_render_target_t *target = user_data;

    if (target)
        pepper_render_target_destroy(target);
}

static pepper_render_target_t *
__tdm_renderer_pixman_get_target(tbm_surface_h surface)
{
    tbm_bo bo;
    pepper_render_target_t *target=NULL;

    bo = tbm_surface_internal_get_bo(surface, 0);
    if(!tbm_bo_get_user_data(bo, KEY_RENDER_TARGET, (void**)&target))
    {
        tbm_surface_info_s info;
        pepper_format_t format;

        tbm_bo_add_user_data(bo, KEY_RENDER_TARGET, __tdm_renderer_pixman_free_target);

        tbm_surface_get_info(surface, &info);
        switch(info.format)
        {
        case TBM_FORMAT_XRGB8888:
            format = PEPPER_FORMAT_XRGB8888;
            break;
        case TBM_FORMAT_ARGB8888:
            format = PEPPER_FORMAT_ARGB8888;
            break;
        default:
            tbm_bo_delete_user_data(bo, KEY_RENDER_TARGET);
            PEPPER_ERROR("Unknown tbm format\n");
            return NULL;
        }
        target = pepper_pixman_renderer_create_target(format, info.planes[0].ptr,
                                                info.planes[0].stride,
                                                info.width, info.height);
        if (!target)
        {
            tbm_bo_delete_user_data(bo, KEY_RENDER_TARGET);
            PEPPER_ERROR("pepper_pixman_renderer_create_target() fail\n");
            return NULL;
        }
        tbm_bo_set_user_data(bo, KEY_RENDER_TARGET, target);
    }

    return target;
}

static void
__tdm_renderer_pixman_render(pepper_tdm_output_t *output)
{
    const pepper_list_t *render_list = pepper_plane_get_render_list(output->primary_plane->base);
    pixman_region32_t   *damage = pepper_plane_get_damage_region(output->primary_plane->base);
    pixman_region32_t    total_damage;
    tbm_surface_h       back;
    pepper_render_target_t *target;

    int ret;

    /*Set render target*/
    ret = tbm_surface_queue_dequeue(output->tbm_surface_queue, &back);
    PEPPER_CHECK(ret == TBM_SURFACE_QUEUE_ERROR_NONE, return, "tbm_surface_queue_dequeue() failed\n");

    target = __tdm_renderer_pixman_get_target(back);
    if (PEPPER_FALSE == pepper_renderer_set_target(output->renderer, target))
    {
        PEPPER_ERROR("pepper_renderer_set_target() failed\n");
        return;
    }

    pixman_region32_init(&total_damage);
    pixman_region32_union(&total_damage, damage, &output->previous_damage);
    pixman_region32_copy(&output->previous_damage, damage);

    pepper_renderer_repaint_output(output->renderer, output->base, render_list, &total_damage);

    pixman_region32_fini(&total_damage);
    pepper_plane_clear_damage_region(output->primary_plane->base);

    output->back = back;
}

static void
__tdm_renderer_pixman_fini(pepper_tdm_output_t *output)
{
    pixman_region32_fini(&output->previous_damage);

    if (output->render_target)
        pepper_render_target_destroy(output->render_target);

    if (output->gbm_surface)
        gbm_surface_destroy(output->gbm_surface);

    output->renderer = NULL;
    output->render_target = NULL;
    output->gbm_surface = NULL;
    output->tbm_surface_queue = NULL;
}

static void
__tdm_renderer_pixman_init(pepper_tdm_output_t *output)
{
    pepper_tdm_t   *tdm = output->tdm;
    const tdm_output_mode *mode;

    if (!tdm->pixman_renderer)
    {
        tdm->pixman_renderer = pepper_pixman_renderer_create(tdm->compositor);
        PEPPER_CHECK(tdm->pixman_renderer, return, "pepper_pixman_renderer_create() failed.\n");
    }

    output->renderer = tdm->pixman_renderer;

    tdm_output_get_mode(output->output, &mode);
    output->gbm_surface = gbm_surface_create(tdm->gbm_device,
                                                mode->width, mode->height, GBM_FORMAT_XRGB8888,
                                                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    PEPPER_CHECK(output->gbm_surface, goto error, "gbm_surface_create() failed.\n");

    output->tbm_surface_queue = gbm_tbm_get_surface_queue(output->gbm_surface);
    PEPPER_CHECK(output->tbm_surface_queue, goto error, "gbm_tbm_get_surface_queue() failed.\n");

    pixman_region32_init(&output->previous_damage);
    output->render_type = TDM_RENDER_TYPE_PIXMAN;

    return;

error:
    __tdm_renderer_pixman_fini(output);
}

static void
__tdm_renderer_gl_render(pepper_tdm_output_t *output)
{
    int ret;

    const pepper_list_t *render_list = pepper_plane_get_render_list(output->primary_plane->base);
    pixman_region32_t   *damage = pepper_plane_get_damage_region(output->primary_plane->base);

    pepper_renderer_repaint_output(output->renderer, output->base, render_list, damage);

    ret = tbm_surface_queue_can_acquire(output->tbm_surface_queue, 1);
    PEPPER_CHECK(ret > 0, return, "tbm_surface_queue_can_acquire() failed.\n");

    ret = tbm_surface_queue_acquire(output->tbm_surface_queue, &output->back);
    PEPPER_CHECK(ret==TBM_SURFACE_QUEUE_ERROR_NONE, return, "tbm_surface_queue_acquire() failed.\n");

    pepper_plane_clear_damage_region(output->primary_plane->base);
}

static void
__tdm_renderer_gl_fini(pepper_tdm_output_t *output)
{
    if (output->render_target)
        pepper_render_target_destroy(output->render_target);

    if (output->gbm_surface)
        gbm_surface_destroy(output->gbm_surface);

    output->renderer = NULL;
    output->render_target = NULL;
    output->gbm_surface = NULL;
    output->tbm_surface_queue = NULL;
}

static void
__tdm_renderer_gl_init(pepper_tdm_output_t *output)
{
    pepper_tdm_t    *tdm = output->tdm;
    const tdm_output_mode *mode;
    uint32_t        native_visual_id = GBM_FORMAT_XRGB8888;

    if (!tdm->gl_renderer)
    {
        tdm->gl_renderer = pepper_gl_renderer_create(tdm->compositor, tdm->gbm_device, "gbm");
        PEPPER_CHECK(tdm->gl_renderer, return, "pepper_gl_renderer_create() failed.\n");
    }

    output->renderer = tdm->gl_renderer;

    tdm_output_get_mode(output->output, &mode);
    output->gbm_surface = gbm_surface_create(tdm->gbm_device,
                                                mode->width, mode->height, GBM_FORMAT_XRGB8888,
                                                GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    PEPPER_CHECK(output->gbm_surface, goto error, "gbm_surface_create() failed.\n");

    output->tbm_surface_queue = gbm_tbm_get_surface_queue(output->gbm_surface);
    PEPPER_CHECK(output->tbm_surface_queue, goto error, "gbm_tbm_get_surface_queue() failed.\n");

    output->render_target = pepper_gl_renderer_create_target(tdm->gl_renderer,
                                                 output->gbm_surface,
                                                 PEPPER_FORMAT_XRGB8888,
                                                 &native_visual_id,
                                                 mode->width, mode->height);
    PEPPER_CHECK(output->render_target, goto error, "pepper_gl_renderer_create_target() failed.\n");
    output->render_type = TDM_RENDER_TYPE_GL;

    pepper_renderer_set_target(output->renderer, output->render_target);
    return;

error:
    __tdm_renderer_gl_fini(output);
}

static void
__tdm_plane_destroy(pepper_event_listener_t *listener, pepper_object_t *object,
                                        uint32_t id, void *info, void *data)
{
    free(data);
}

static int
__tdm_output_plane_init(pepper_tdm_output_t *output)
{
    int num_layer, i;
    pepper_tdm_plane_t *plane;
    pepper_plane_t *prev=NULL;
    const tdm_output_mode *mode;
    tdm_info_layer info;

    tdm_error err;

    err = tdm_output_get_mode(output->output, &mode);
    PEPPER_CHECK(err == TDM_ERROR_NONE, return PEPPER_FALSE, "tdm_output_get_mode()\n");

    /*TODO : set default layer info*/
    info.transform = TDM_TRANSFORM_NORMAL;
    info.dst_pos.x = 0;
    info.dst_pos.y = 0;
    info.dst_pos.w = mode->width;
    info.dst_pos.h = mode->height;
    info.src_config.size.h = mode->width;
    info.src_config.size.v = mode->height;
    info.src_config.pos.x = 0;
    info.src_config.pos.y = 0;
    info.src_config.pos.w = mode->width;
    info.src_config.pos.h = mode->height;

    tdm_output_get_layer_count(output->output, &num_layer);
    PEPPER_CHECK(num_layer > 0, return PEPPER_FALSE, "number of tdm_layer is 0\n");

    for (i=0; i<num_layer; i++)
    {
        plane = (pepper_tdm_plane_t *)calloc(1, sizeof(pepper_tdm_plane_t));
        PEPPER_CHECK(plane, return PEPPER_FALSE, "calloc failed\n");

        plane->layer = (tdm_layer*)tdm_output_get_layer(output->output, i, &err);
        PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                "tdm_output_get_layer failed err:%d\n", err);

        plane->base = pepper_output_add_plane(output->base, prev);
        PEPPER_CHECK(plane->base, goto error,
                "pepper_output_add_plane() failed\n");
        pepper_object_add_event_listener((pepper_object_t *)plane->base,
                                    PEPPER_EVENT_OBJECT_DESTROY,
                                    0,
                                    __tdm_plane_destroy, plane);

        tdm_layer_get_capabilities(plane->layer, &plane->caps);
        if (plane->caps & TDM_LAYER_CAPABILITY_PRIMARY)
        {
            output->primary_plane = plane;
        }
        tdm_layer_set_info(plane->layer, &info);
    }

    if (!output->primary_plane)
    {
        PEPPER_ERROR("primary plane is NULL\n");
        return PEPPER_FALSE;
    }

    return PEPPER_TRUE;

error:
    if (plane->base)
        pepper_plane_destroy(plane->base);
    else
        free(plane);

    return PEPPER_FALSE;
}





static void
__tdm_output_free_tdm_buffer(void *user_data)
{
    /* TODO */
}

static tdm_buffer *
__tdm_output_get_tdm_buffer(tbm_surface_h surface)
{
    tbm_bo bo;
    tdm_buffer *target = NULL;
    tdm_error err;

    bo = tbm_surface_internal_get_bo(surface, 0);
    if(!tbm_bo_get_user_data(bo, KEY_TDM_BUFFER, (void**)&target))
    {
        tbm_bo_add_user_data(bo, KEY_TDM_BUFFER, __tdm_output_free_tdm_buffer);

        target = tdm_buffer_create(surface, &err);
        if (err != TDM_ERROR_NONE)
        {
            tbm_bo_delete_user_data(bo, KEY_TDM_BUFFER);
            return NULL;
        }

        tbm_bo_set_user_data(bo, KEY_TDM_BUFFER, target);
    }

    return target;
}



static void
__tdm_output_commit_cb(tdm_output *o, unsigned int sequence,
                            unsigned int tv_sec, unsigned int tv_usec, void *user_data)
{
    pepper_tdm_output_t *output = user_data;
    struct timespec     ts;

    if (output->page_flip_pending == PEPPER_TRUE)
    {
        output->page_flip_pending = PEPPER_FALSE;

        if (output->front)
        {
            tbm_surface_queue_release(output->tbm_surface_queue, output->front);
        }

        output->front = output->back;
        output->back = NULL;
    }

    ts.tv_sec = tv_sec;
    ts.tv_nsec = tv_usec * 1000;
    pepper_output_finish_frame(output->base, &ts);
}

static void
pepper_tdm_output_destroy(void *o)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;

    /* TODO: */

    free(output);
}

static int32_t
pepper_tdm_output_get_subpixel_order(void *o)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    unsigned int subpixel;

    tdm_output_get_subpixel(output->output, &subpixel);
    switch(subpixel)
    {
    default:
        subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
    }

    return subpixel;
}

static const char *
pepper_tdm_output_get_maker_name(void *o)
{
    return "tizen.org";
}

static const char *
pepper_tdm_output_get_model_name(void *o)
{
    return "TDM";
}

static int
pepper_tdm_output_get_mode_count(void *o)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    int num_mode;
    const tdm_output_mode *modes;

    tdm_output_get_available_modes(output->output, &modes, &num_mode);
    return num_mode;
}

static void
pepper_tdm_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    int num_mode;
    const tdm_output_mode *modes;
    const tdm_output_mode *cur_mode;

    tdm_output_get_available_modes(output->output, &modes, &num_mode);
    PEPPER_CHECK(index < num_mode, return, "mode index is invalid\n");

    mode->flags = 0;
    mode->w = modes[index].width;
    mode->h = modes[index].height;
    mode->refresh = modes[index].refresh;

    if (modes[index].type & TDM_OUTPUT_MODE_TYPE_PREFERRED)
        mode->flags |= WL_OUTPUT_MODE_PREFERRED;

    tdm_output_get_mode(output->output, &cur_mode);
    if (cur_mode == &modes[index])
        mode->flags |= WL_OUTPUT_MODE_CURRENT;
}

static pepper_bool_t
pepper_tdm_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    int num_mode, i;
    const tdm_output_mode *modes;
    tdm_error err;

    tdm_output_get_available_modes(output->output, &modes, &num_mode);
    for (i=0; i<num_mode; i++)
    {
        if (mode->w == (int32_t)modes[i].width &&
            mode->h == (int32_t)modes[i].height &&
            mode->refresh == (int32_t)modes[i].refresh)
        {
            err = tdm_output_set_mode(output->output, (tdm_output_mode*)&modes[i]);
            PEPPER_CHECK(err == TDM_ERROR_NONE, return PEPPER_FALSE,
                    "tdm_output_set_mode() failed mode:%s\n", modes[i].name);
            return PEPPER_TRUE;
        }
    }

    return PEPPER_FALSE;
}

static void
pepper_tdm_output_assign_planes(void *o, const pepper_list_t *view_list)
{
    pepper_list_t      *l;
    pepper_tdm_output_t       *output = o;

    pepper_list_for_each_list(l, view_list)
    {
        pepper_view_t      *view = l->item;
        pepper_plane_t     *plane = NULL;

        plane = output->primary_plane->base;

        pepper_view_assign_plane(view, output->base, plane);
    }
}

static void
pepper_tdm_output_start_repaint_loop(void *o)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    struct timespec     ts;

    pepper_compositor_get_time(output->tdm->compositor, &ts);
    pepper_output_finish_frame(output->base, &ts);
}

static void
pepper_tdm_output_repaint(void *o, const pepper_list_t *plane_list)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    tdm_error err;

    if (!output->back)
    {
        if (output->render_type == TDM_RENDER_TYPE_GL)
            __tdm_renderer_gl_render(output);
        else if (output->render_type == TDM_RENDER_TYPE_PIXMAN)
            __tdm_renderer_pixman_render(output);
        else
        {
            PEPPER_ERROR("Unknown render type\n");
            return;
        }
    }

    if (output->back)
    {
        tdm_buffer *tdm_buf;
        tdm_buf = __tdm_output_get_tdm_buffer(output->back);
        PEPPER_CHECK(tdm_buf, return, "__tdm_output_get_tdm_buffer() failed");

        err = tdm_layer_set_buffer((tdm_layer*)output->primary_plane->layer, tdm_buf);
        PEPPER_CHECK(err == TDM_ERROR_NONE, return, "tdm_layer_set_buffer() failed");

        err = tdm_output_commit(output->output, 0, __tdm_output_commit_cb, output);
        PEPPER_CHECK(err == TDM_ERROR_NONE, return, "tdm_output_commit() failed");

        output->page_flip_pending = PEPPER_TRUE;
    }
}

static void
pepper_tdm_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
    pepper_tdm_output_t *output = (pepper_tdm_output_t *)o;
    pepper_renderer_attach_surface(output->renderer, surface, w, h);
}

static void
pepper_tdm_output_flush_surface_damage(void *o, pepper_surface_t *surface, pepper_bool_t *keep_buffer)
{
    pepper_tdm_output_t    *output = o;
    pepper_buffer_t *buffer = pepper_surface_get_buffer(surface);

    pepper_renderer_flush_surface_damage(output->renderer, surface);

    if (output->render_type == TDM_RENDER_TYPE_GL &&
        (buffer && wl_shm_buffer_get(pepper_buffer_get_resource(buffer))))
    {
        *keep_buffer = PEPPER_FALSE;
    }
    else
    {
        *keep_buffer = PEPPER_TRUE;
    }
}

struct pepper_output_backend tdm_output_backend =
{
    pepper_tdm_output_destroy,

    pepper_tdm_output_get_subpixel_order,
    pepper_tdm_output_get_maker_name,
    pepper_tdm_output_get_model_name,

    pepper_tdm_output_get_mode_count,
    pepper_tdm_output_get_mode,
    pepper_tdm_output_set_mode,

    pepper_tdm_output_assign_planes,
    pepper_tdm_output_start_repaint_loop,
    pepper_tdm_output_repaint,
    pepper_tdm_output_attach_surface,
    pepper_tdm_output_flush_surface_damage,
};

int
pepper_tdm_output_init(pepper_tdm_t *tdm)
{
    pepper_tdm_output_t *output;
    tdm_error err;
    int i;

    int num_output;
    tdm_output_conn_status status;

    const tdm_output_mode *modes;
    const tdm_output_mode *default_mode = NULL;
    const tdm_output_mode *preferred_mode = NULL;
    int num_mode;

    const char     *render_env = getenv("PEPPER_RENDERER");

    tdm_display_get_output_count(tdm->disp, &num_output);
    PEPPER_CHECK(num_output > 0, return PEPPER_FALSE, "Number of output is 0\n");

    while(num_output--)
    {
        output = (pepper_tdm_output_t*)calloc(1, sizeof(pepper_tdm_output_t));
        output->tdm = tdm;
        output->output = (tdm_output *)tdm_display_get_output(tdm->disp, num_output, &err);
        PEPPER_CHECK((output->output && (err == TDM_ERROR_NONE)), goto error,
                    "tdm_display_get_output(%d) failed err:%d\n", num_output, err);

        /*Check connection state*/
        err = tdm_output_get_conn_status(output->output, &status);
        PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                    "tdm_display_get_output(%d) failed err:%d\n", num_output, err);
        if (status != TDM_OUTPUT_CONN_STATUS_CONNECTED)
            continue;


        /*Setup default mode*/
        err = tdm_output_get_available_modes(output->output, &modes, &num_mode);
        PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                   "tdm_output_get_available_modes(%d) failed err:%d\n", num_output, err);
        for (i=0; i<num_mode; i++)
        {
            if (modes[i].type & TDM_OUTPUT_MODE_TYPE_PREFERRED)
                preferred_mode = &modes[i];

            if (modes[i].type & TDM_OUTPUT_MODE_TYPE_DEFAULT)
                default_mode = &modes[i];
        }

        if (preferred_mode)
        {
            err = tdm_output_set_mode(output->output, (tdm_output_mode *)preferred_mode);
            PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                        "tdm_output_set_mode() failed err:%d\n",err);
        }
        else if(default_mode)
        {
            err = tdm_output_set_mode(output->output, (tdm_output_mode *)default_mode);
            PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                        "tdm_output_set_mode() failed err:%d\n",err);
        }
        else
        {
            err = tdm_output_set_mode(output->output, (tdm_output_mode *)&modes[0]);
            PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                        "tdm_output_set_mode() failed err:%d\n",err);
        }

        /*Setup renderer*/
        if (render_env && !strcmp(render_env, "gl"))
        {
            __tdm_renderer_gl_init(output);
            PEPPER_CHECK(output->renderer, goto error, "Failed to initialize gl_renderer.\n");
        }

        if (!output->renderer)
        {
            __tdm_renderer_pixman_init(output);
            PEPPER_CHECK(output->renderer, goto error, "Failed to initialize pixman_renderer.\n");
        }

        /*Add pepper_output to compositor*/
        output->base = pepper_compositor_add_output(tdm->compositor,
                                                &tdm_output_backend,
                                                "tdm_output",
                                                output,
                                                WL_OUTPUT_TRANSFORM_NORMAL, 1);
        PEPPER_CHECK(err == TDM_ERROR_NONE, goto error,
                    "pepper_compositor_add_output() failed err:%d\n",err);

        PEPPER_CHECK(PEPPER_TRUE == __tdm_output_plane_init(output), goto error,
                    "pepper_tdm_plane_init() failed\n");
    }

    return PEPPER_TRUE;

    error:
    if (output->base)
        pepper_output_destroy(output->base);
    else
        free(output);

    return PEPPER_FALSE;
}
