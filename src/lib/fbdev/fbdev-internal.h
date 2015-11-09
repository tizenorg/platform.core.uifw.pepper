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

#ifndef FBDEV_INTERNAL_H
#define FBDEV_INTERNAL_H

#include <pixman.h>

#include <pepper-output-backend.h>
#include <pepper-libinput.h>
#include <pepper-render.h>
#include <pepper-pixman-renderer.h>

#include "pepper-fbdev.h"

typedef struct fbdev_output     fbdev_output_t;

struct pepper_fbdev
{
    pepper_compositor_t        *compositor;

    pepper_list_t               output_list;

    uint32_t                    min_width, min_height;
    uint32_t                    max_width, max_height;

    struct udev                *udev;

    pepper_renderer_t          *pixman_renderer;
};

struct fbdev_output
{
    pepper_fbdev_t             *fbdev;
    pepper_output_t            *base;
    pepper_list_t               link;

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

    struct wl_event_source     *frame_done_timer;

    pepper_plane_t             *primary_plane;
    /* TODO */
};

pepper_bool_t
pepper_fbdev_output_create(pepper_fbdev_t *fbdev, const char *renderer);

void
pepper_fbdev_output_destroy(fbdev_output_t *output);

#endif /* FBDEV_INTERNAL_H */
