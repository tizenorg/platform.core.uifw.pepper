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

#include <config.h>
#include <fcntl.h>
#include <unistd.h>
#include "tdm-internal.h"

static int
__tdm_handle_event(int fd, uint32_t mask, void *data)
{
	pepper_tdm_t *tdm = data;

	tdm_display_handle_events(tdm->disp);
	return 0;
}

PEPPER_API pepper_tdm_t *
pepper_tdm_create(pepper_compositor_t *compositor)
{
	tdm_error              ret;
	pepper_tdm_t           *tdm = NULL;
	struct wl_event_loop   *loop;

	tdm = calloc(1, sizeof(pepper_tdm_t));
	PEPPER_CHECK(tdm, return NULL, "calloc() failed.\n");

	tdm->fd = -1;
	tdm->compositor = compositor;
	tdm->disp = tdm_display_init(&ret);
	PEPPER_CHECK(ret == TDM_ERROR_NONE, goto error,
				 "tdm_display_init() failed %d\n", ret);

	ret = tdm_display_get_fd(tdm->disp, &tdm->fd);
	PEPPER_CHECK(ret == TDM_ERROR_NONE, goto error,
				 "tdm_display_get_fd() failed %d\n", ret);

	tdm->bufmgr = tbm_bufmgr_init(tdm->fd);
	PEPPER_CHECK(tdm->bufmgr, goto error, "tbm_bufmgr_init() failed \n");

#ifdef HAVE_TBM
	/* Create wayland-tbm
	          FIXME : Cannot get filepath for tbm
	     */

	tdm->wl_tbm_server = wayland_tbm_server_init(pepper_compositor_get_display(
							 compositor),
						 "/dev/dri/card0", tdm->fd, 0);
#endif

	/*Setup outputs*/
	pepper_tdm_output_init(tdm);

	/* Add TDM event FDs to the compositor's display. */
	loop = wl_display_get_event_loop(pepper_compositor_get_display(compositor));

	tdm->tdm_event_source = wl_event_loop_add_fd(loop, tdm->fd, WL_EVENT_READABLE,
							__tdm_handle_event, tdm);
	PEPPER_CHECK(tdm->tdm_event_source, goto error,
				 "wl_event_loop_add() failed.\n");

	if (!pepper_compositor_set_clock_id(compositor, CLOCK_MONOTONIC))
		goto error;

	return tdm;

error:
	pepper_tdm_destroy(tdm);

	return NULL;
}

PEPPER_API void
pepper_tdm_destroy(pepper_tdm_t *tdm)
{
	if (tdm->tdm_event_source)
		wl_event_source_remove(tdm->tdm_event_source);

	if (tdm->wl_tbm_server)
		wayland_tbm_server_deinit(tdm->wl_tbm_server);

	if (tdm->bufmgr)
		tbm_bufmgr_deinit(tdm->bufmgr);

	if (tdm->disp)
		tdm_display_deinit(tdm->disp);

	if (tdm->fd != -1)
		close(tdm->fd);

	free(tdm);
}

