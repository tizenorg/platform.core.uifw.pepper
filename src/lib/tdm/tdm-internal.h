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

#ifndef TDM_INTERNAL_H
#define TDM_INTERNAL_H

#include <unistd.h>
#include <config.h>
#include <pixman.h>
#include <tdm.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_queue.h>
#ifdef HAVE_TBM
#include <wayland-tbm-server.h>
#endif
#include <pepper-output-backend.h>
#include <pepper-libinput.h>
#include <pepper-render.h>

#include "pepper-tdm.h"

typedef struct pepper_tdm_output        pepper_tdm_output_t;
typedef struct pepper_tdm_plane         pepper_tdm_plane_t;

typedef enum tdm_render_type
{
    TDM_RENDER_TYPE_PIXMAN,
    TDM_RENDER_TYPE_GL,
} tdm_render_type_t;

struct pepper_tdm
{
    pepper_compositor_t        *compositor;
    tdm_display                *disp;
    tbm_bufmgr                  bufmgr;
    int                         fd;

    pepper_list_t               output_list;

    struct wayland_tbm_server  *wl_tbm_server;
    struct wl_event_source     *tdm_event_source;

    pepper_renderer_t          *pixman_renderer;
    pepper_renderer_t          *gl_renderer;
};

struct pepper_tdm_output
{
    pepper_output_t        *base;
    pepper_tdm_t           *tdm;
    pepper_tdm_output_t    *output;
    pepper_tdm_plane_t     *primary_plane;

    tbm_surface_queue_h     tbm_surface_queue;

    tdm_render_type_t       render_type;
    pepper_renderer_t      *renderer;
    pepper_render_target_t *render_target;

    tbm_surface_h           back, front;
    pepper_bool_t           page_flip_pending;
    /*For pixman*/
    pixman_region32_t       previous_damage;
};

struct pepper_tdm_plane
{
    pepper_plane_t         *base;
    pepper_tdm_output_t    *output;
    tdm_layer              *layer;
    tdm_layer_capability    caps;
};

int pepper_tdm_output_init(pepper_tdm_t *tdm);
#endif /* DRM_INTERNAL_H */
