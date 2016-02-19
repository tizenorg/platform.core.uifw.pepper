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

#ifndef PEPPER_RENDER_INTERNAL_H
#define PEPPER_RENDER_INTERNAL_H

#include "pepper-render.h"

struct pepper_render_target {
	/* Renderer from where this target is created. */
	pepper_renderer_t   *renderer;

	void    (*destroy)(pepper_render_target_t *target);
};

struct pepper_renderer {
	pepper_compositor_t    *compositor;
	pepper_render_target_t *target;

	void            (*destroy)(pepper_renderer_t *renderer);

	pepper_bool_t   (*attach_surface)(pepper_renderer_t *renderer,
	                                  pepper_surface_t *surface, int *w, int *h);

	pepper_bool_t   (*flush_surface_damage)(pepper_renderer_t *renderer,
	                                        pepper_surface_t *surface);

	pepper_bool_t   (*read_pixels)(pepper_renderer_t *renderer,
	                               int x, int y, int w, int h,
	                               void *pixels, pepper_format_t format);

	void            (*repaint_output)(pepper_renderer_t *renderer,
	                                  pepper_output_t *output,
	                                  const pepper_list_t *render_list,
	                                  pixman_region32_t *damage);
};

#endif /* PEPPER_RENDER_INTERNAL_H */
