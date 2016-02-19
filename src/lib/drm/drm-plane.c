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

#include "drm-internal.h"

void
drm_init_planes(pepper_drm_t *drm)
{
	int                 i;
	drmModePlaneRes    *res = drmModeGetPlaneResources(drm->fd);

	PEPPER_CHECK(res, return, "drmModeGetPlaneResources() failed.\n");

	for (i = 0; i < (int)res->count_planes; i++) {
		drm_plane_t *plane = calloc(1, sizeof(drm_plane_t));
		PEPPER_CHECK(plane, continue, "calloc() failed.\n");

		plane->plane = drmModeGetPlane(drm->fd, res->planes[i]);
		if (!plane->plane) {
			PEPPER_ERROR("drmModeGetPlane() failed.\n");
			free(plane);
			continue;
		}
		plane->drm = drm;
		plane->id = plane->plane->plane_id;

		pepper_list_insert(drm->plane_list.prev, &plane->link);
	}

	drmModeFreePlaneResources(res);
}

void
drm_plane_destroy(drm_plane_t *plane)
{
	if (plane->plane)
		drmModeFreePlane(plane->plane);

	pepper_list_remove(&plane->link);
	free(plane);
}
