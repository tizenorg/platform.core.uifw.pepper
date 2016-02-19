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

#include "pepper-internal.h"

/**
 * Transforms a pixman region from global space to output local space
 *
 * @param region    pixman region
 * @param output    output object
 */
PEPPER_API void
pepper_pixman_region_global_to_output(pixman_region32_t *region,
				      pepper_output_t *output)
{
	pixman_box32_t *box, b;
	int             num_rects, i;
	int32_t         scale = output->scale;
	int32_t         w = output->geometry.w;
	int32_t         h = output->geometry.h;

	/* Transform into output geometry origin. */
	pixman_region32_translate(region, output->geometry.x, output->geometry.y);

	if (output->geometry.transform == WL_OUTPUT_TRANSFORM_NORMAL && scale == 1)
		return;

	box = pixman_region32_rectangles(region, &num_rects);

	switch (output->geometry.transform) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
		break;
	case WL_OUTPUT_TRANSFORM_90:
		for (i = 0; i < num_rects; i++) {
			b.x1 = h - box[i].y2;
			b.y1 = box[i].x1;
			b.x2 = h - box[i].y1;
			b.y2 = box[i].x2;

			box[i] = b;
		}
		break;
	case WL_OUTPUT_TRANSFORM_180:
		for (i = 0; i < num_rects; i++) {
			b.x1 = w - box[i].x2;
			b.y1 = h - box[i].y2;
			b.x2 = w - box[i].x1;
			b.y2 = h - box[i].y1;

			box[i] = b;
		}
		break;
	case WL_OUTPUT_TRANSFORM_270:
		for (i = 0; i < num_rects; i++) {
			b.x1 = box[i].y1;
			b.y1 = w - box[i].x2;
			b.x2 = box[i].y2;
			b.y2 = w - box[i].x1;

			box[i] = b;
		}
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED:
		for (i = 0; i < num_rects; i++) {
			b.x1 = w - box[i].x2;
			b.y1 = box[i].y1;
			b.x2 = w - box[i].x1;
			b.y2 = box[i].y2;

			box[i] = b;
		}
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		for (i = 0; i < num_rects; i++) {
			b.x1 = h - box[i].y2;
			b.y1 = w - box[i].x2;
			b.x2 = h - box[i].y1;
			b.y2 = w - box[i].x1;

			box[i] = b;
		}
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		for (i = 0; i < num_rects; i++) {
			b.x1 = box[i].x1;
			b.y1 = h - box[i].y2;
			b.x2 = box[i].x2;
			b.y2 = h - box[i].y1;

			box[i] = b;
		}
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		for (i = 0; i < num_rects; i++) {
			b.x1 = box[i].y1;
			b.y1 = box[i].x1;
			b.x2 = box[i].y2;
			b.y2 = box[i].x2;

			box[i] = b;
		}
		break;
	}

	if (scale != 1) {
		for (i = 0; i < num_rects; i++) {
			box[i].x1 *= scale;
			box[i].y1 *= scale;
			box[i].x2 *= scale;
			box[i].y2 *= scale;
		}
	}
}

/**
 * Transforms coordinates from surface local to buffer local space
 *
 * @param surface   surface object
 * @param sx        x coordinate in surface local space
 * @param sy        y coordinate in surface local space
 * @param bx        pointer to receive x coordinate in buffer local space
 * @param by        pointer to receive y coordinate in buffer local space
 */
PEPPER_API void
pepper_coordinates_surface_to_buffer(pepper_surface_t *surface,
				     double sx, double sy, double *bx, double *by)
{
	int32_t             scale, w, h;

	scale = surface->buffer.scale;
	w = surface->w;
	h = surface->h;

	switch (surface->buffer.transform) {
	case WL_OUTPUT_TRANSFORM_NORMAL:
		*bx = sx - surface->buffer.x;
		*by = sy - surface->buffer.y;
		break;
	case WL_OUTPUT_TRANSFORM_90:
		*bx = h - (sy - surface->buffer.y);
		*by = sx - surface->buffer.x;
		break;
	case WL_OUTPUT_TRANSFORM_180:
		*bx = w - (sx - surface->buffer.x);
		*by = h - (sy - surface->buffer.y);
		break;
	case WL_OUTPUT_TRANSFORM_270:
		*bx = sy - surface->buffer.y;
		*by = w - (sx - surface->buffer.x);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED:
		*bx = w - (sx - surface->buffer.x);
		*by = sy - surface->buffer.y;
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		*bx = h - (sy - surface->buffer.y);
		*by = w - (sx - surface->buffer.x);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		*bx = sx - surface->buffer.x;
		*by = h - (sy - surface->buffer.y);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		*bx = sy - surface->buffer.y;
		*by = sx - surface->buffer.x;
		break;
	}

	if (scale != 1) {
		*bx *= scale;
		*by *= scale;
	}
}

/* Calculate a matrix which transforms vertices into the output local space,
 * so that output backends can simply use the matrix to transform a view
 * into the frame buffer space.
 *
 * view space -> global space -> output space
 *
 * global to output space.
 *
 * 1. Translate to output geometry origin. (denote T)
 * 2. Apply output transform. (denote X)
 * 3. Apply output scale. (denote S)
 *
 * P' = S * X * T * G * P
 *
 * Given a global matrix G, we should calculate matrix M where
 * M = S * X * T * G
 *
 * S can be represented as a single scale term.
 * X can be represented using 6 values (a, b, c, d, e, f) where
 *   x' = a * x + b * y + e;
 *   y' = c * x + b * y + f;
 * T can be represented using 2 translation term (x, y)
 */
static inline void
make_output_transform(pepper_mat4_t *mat,
		      double s, /* scale */
		      double a, double b, double c, double d, double e, double f, /* transform */
		      double x, double y /* translate */)
{
	double *m = &mat->m[0];

	m[ 0] = s * a;
	m[ 1] = s * c;
	m[ 4] = s * b;
	m[ 5] = s * d;
	m[12] = s * (a * x + b * y + e);
	m[13] = s * (c * x + d * y + f);

	if (s != 1.0)
		mat->flags |= PEPPER_MATRIX_SCALE;

	if (x != 0.0 || y != 0.0)
		mat->flags |= PEPPER_MATRIX_TRANSLATE;

	if (a != 1.0 || d != 1.0)
		mat->flags |= PEPPER_MATRIX_ROTATE;
}

void
pepper_transform_global_to_output(pepper_mat4_t *transform,
				  pepper_output_t *output)
{
	pepper_mat4_t   mat;
	double          x = output->geometry.x;
	double          y = output->geometry.y;
	double          w = output->geometry.w;
	double          h = output->geometry.h;

	pepper_mat4_init_identity(&mat);

	switch (output->geometry.transform) {
	default:
	case WL_OUTPUT_TRANSFORM_NORMAL:
		make_output_transform(&mat, (double)output->scale, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED:
		make_output_transform(&mat, (double)output->scale, -1.0, 0.0, 0.0, 1.0, w, 0.0,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_90:
		make_output_transform(&mat, (double)output->scale, 0.0, -1.0, 1.0, 0.0, h, 0.0,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
		make_output_transform(&mat, (double)output->scale, 0.0, -1.0, -1.0, 0.0, h, w,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_180:
		make_output_transform(&mat, (double)output->scale, -1.0, 0.0, 0.0, -1.0, w, h,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_180:
		make_output_transform(&mat, (double)output->scale, 1.0, 0.0, 0.0, -1.0, 0.0, h,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_270:
		make_output_transform(&mat, (double)output->scale, 0.0, 1.0, -1.0, 0.0, 0.0,  w,
				      x, y);
		break;
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		make_output_transform(&mat, (double)output->scale, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0,
				      x, y);
		break;
	}

	pepper_mat4_multiply(transform, &mat, transform);
}
