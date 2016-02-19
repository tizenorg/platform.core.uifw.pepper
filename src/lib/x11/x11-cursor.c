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
#include <stdlib.h>

static void *
x11_output_cursor_set(void *o, void *c)
{
	x11_output_t     *output = o;
	x11_cursor_t     *cursor = c;
	x11_cursor_t     *current_cursor;
	pepper_x11_connection_t *conn;

	if (!output) {
		PEPPER_ERROR("x11:output:cursor:%s output is null\n", __FUNCTION__);
		return NULL;
	}

	if (!cursor) {
		PEPPER_ERROR("x11:output:cursor:%s cursor is null\n", __FUNCTION__);
		return NULL;
	}

	conn = output->connection;

	current_cursor = output->cursor;
	output->cursor = cursor;

	/* set cursor for window*/
	{
		uint32_t mask       = XCB_CW_CURSOR;
		uint32_t value_list = cursor->xcb_cursor;
		xcb_change_window_attributes(conn->xcb_connection,
		                             output->window,
		                             mask,
		                             &value_list);
	}

	return current_cursor;
}

static void *
x11_output_cursor_create(void *output, int32_t w, int32_t h, void *image)
{
	xcb_pixmap_t     pixmap;
	xcb_gc_t         gc;
	pepper_x11_connection_t *conn;
	x11_cursor_t     *cursor;

	if (!output) {
		PEPPER_ERROR("x11:output:cursor:%s: output is null\n", __FUNCTION__);
		return NULL;
	}
	if (!image) {
		PEPPER_ERROR("x11:output:cursor:%s: image is null\n", __FUNCTION__);
		return NULL;
	}
	if (w < 0 || h < 0) {
		PEPPER_ERROR("x11:output:cursor:%s: width(%d) or height(%d) is invalid\n",
		             __FUNCTION__, w, h);
		return NULL;
	}

	cursor = calloc(1, sizeof(x11_cursor_t));
	if (!cursor) {
		PEPPER_ERROR("x11:cursor: memory allocation failed");
		return NULL;
	}
	cursor->data = image;
	cursor->w = w;
	cursor->h = h;

	conn = ((x11_output_t *)output)->connection;

	pixmap = xcb_generate_id(conn->xcb_connection);
	gc = xcb_generate_id(conn->xcb_connection);

	xcb_create_pixmap(conn->xcb_connection, 1/*depth?*/,
	                  pixmap, conn->screen->root, w, h);
	xcb_create_gc(conn->xcb_connection, gc, pixmap, 0, NULL);
	xcb_put_image(conn->xcb_connection, XCB_IMAGE_FORMAT_XY_PIXMAP, pixmap,
	              gc, w, h, 0, 0, 0, 32, w * h * sizeof(uint8_t), cursor->data);
	cursor->xcb_cursor = xcb_generate_id(conn->xcb_connection);
	/*
	 * cb_void_cookie_t xcb_create_cursor(xcb_connection_t *conn,
	 *                                    xcb_cursor_t cid,
	 *                                    xcb_pixmap_t source,
	 *                                    xcb_pixmap_t mask,
	 *                                    uint16_t fore_red,    TODO: NOT YET DOCUMENTED.
	 *                                    uint16_t fore_green,
	 *                                    uint16_t fore_blue,
	 *                                    uint16_t back_red,
	 *                                    uint16_t back_green,
	 *                                    uint16_t back_blue,
	 *                                    uint16_t x,
	 *                                    uint16_t y);
	 */
	xcb_create_cursor(conn->xcb_connection, cursor->xcb_cursor,
	                  pixmap, pixmap, 0, 0, 0,  0, 0, 0,  1, 1);

	xcb_free_gc(conn->xcb_connection, gc);
	xcb_free_pixmap(conn->xcb_connection, pixmap);

	return (void *)cursor;
}

static void
x11_output_cursor_destroy(void *o, void *c)
{
	xcb_connection_t *conn;
	x11_cursor_t     *cursor;

	if (!o) {
		PEPPER_ERROR("x11:output:cursor:%s: output is null\n", __FUNCTION__);
		return ;
	}
	if (!c) {
		PEPPER_ERROR("x11:output:cursor:%s: cursor is null\n", __FUNCTION__);
		return ;
	}

	conn = ((x11_output_t *)o)->connection->xcb_connection;
	cursor = (x11_cursor_t *)c;

	xcb_free_cursor(conn, cursor->xcb_cursor);
	free(cursor);
}
