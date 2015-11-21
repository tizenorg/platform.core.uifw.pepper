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

#include "pepper-render-internal.h"

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer)
{
    renderer->destroy(renderer);
}

PEPPER_API void
pepper_render_target_destroy(pepper_render_target_t *target)
{
    target->destroy(target);
}

PEPPER_API pepper_bool_t
pepper_renderer_set_target(pepper_renderer_t *renderer, pepper_render_target_t *target)
{
    if (target->renderer != NULL && target->renderer != renderer)
        return PEPPER_FALSE;

    renderer->target = target;
    return PEPPER_TRUE;
}

PEPPER_API pepper_render_target_t *
pepper_renderer_get_target(pepper_renderer_t *renderer)
{
    return renderer->target;
}

PEPPER_API pepper_bool_t
pepper_renderer_attach_surface(pepper_renderer_t *renderer,
                               pepper_surface_t *surface, int *w, int *h)
{
    return renderer->attach_surface(renderer, surface, w, h);
}

PEPPER_API pepper_bool_t
pepper_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_surface_t *surface)
{
    return renderer->flush_surface_damage(renderer, surface);
}

PEPPER_API void
pepper_renderer_repaint_output(pepper_renderer_t *renderer, pepper_output_t *output,
                               const pepper_list_t *view_list, pixman_region32_t *damage)
{
    renderer->repaint_output(renderer, output, view_list, damage);
}

PEPPER_API pepper_bool_t
pepper_renderer_read_pixels(pepper_renderer_t *renderer, int x, int y, int w, int h,
                            void *pixels, pepper_format_t format)
{
    return renderer->read_pixels(renderer, x, y, w, h, pixels, format);
}
