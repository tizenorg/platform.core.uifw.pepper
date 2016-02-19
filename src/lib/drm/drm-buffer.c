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
#include <sys/mman.h>
#include <pepper-pixman-renderer.h>

drm_buffer_t *
drm_buffer_create_dumb(pepper_drm_t *drm, uint32_t w, uint32_t h)
{
	drm_buffer_t                *buffer;
	struct drm_mode_create_dumb  create_arg;
	struct drm_mode_map_dumb     map_arg;
	int                          ret;

	buffer = calloc(1, sizeof(drm_buffer_t));
	PEPPER_CHECK(buffer, return NULL, "calloc() failed.\n");

	buffer->drm = drm;
	buffer->type = DRM_BUFFER_TYPE_DUMB;
	buffer->w = w;
	buffer->h = h;

	memset(&create_arg, 0x00, sizeof(create_arg));
	create_arg.bpp = 32;
	create_arg.width = w;
	create_arg.height = h;

	ret = drmIoctl(drm->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg);
	PEPPER_CHECK(ret == 0, goto error, "drmIoctl() failed.\n");

	buffer->handle = create_arg.handle;
	buffer->stride = create_arg.pitch;
	buffer->size   = create_arg.size;

	ret = drmModeAddFB(drm->fd, w, h, 24, 32, buffer->stride, buffer->handle,
	                   &buffer->id);
	PEPPER_CHECK(ret == 0, goto error, "drmModeAddFB() failed.\n");

	memset(&map_arg, 0, sizeof(map_arg));
	map_arg.handle = buffer->handle;

	ret = drmIoctl(drm->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_arg);
	PEPPER_CHECK(ret == 0, goto error, "drmIoctl() failed.\n");

	buffer->map = mmap(0, buffer->size, PROT_WRITE, MAP_SHARED, drm->fd,
	                   map_arg.offset);
	PEPPER_CHECK(buffer->map != MAP_FAILED, goto error, "mmap() failed.\n");

	buffer->image = pixman_image_create_bits(PIXMAN_x8r8g8b8, w, h, buffer->map,
	                buffer->stride);
	PEPPER_CHECK(buffer->image, goto error, "pixman_image_create_bits() failed.\n");

	return buffer;

error:
	if (buffer->map)
		munmap(buffer->map, buffer->size);

	if (buffer->id)
		drmModeRmFB(drm->fd, buffer->id);

	if (buffer->handle) {
		struct drm_mode_destroy_dumb destroy_arg;

		memset(&destroy_arg, 0x00, sizeof(destroy_arg));
		destroy_arg.handle = buffer->handle;
		drmIoctl(drm->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg);
	}

	if (buffer->image)
		pixman_image_unref(buffer->image);

	free(buffer);
	return NULL;
}

static inline pepper_bool_t
init_buffer_gbm(drm_buffer_t *buffer, pepper_drm_t *drm, struct gbm_bo *bo,
                uint32_t format)
{
	int         ret;
	uint32_t    handles[4], strides[4], offsets[4];

	buffer->drm = drm;
	buffer->handle = gbm_bo_get_handle(bo).u32;
	buffer->w = gbm_bo_get_width(bo);
	buffer->h = gbm_bo_get_height(bo);
	buffer->stride = gbm_bo_get_stride(bo);
	buffer->size = buffer->h * buffer->stride;

	handles[0] = buffer->handle;
	strides[0] = buffer->stride;
	offsets[0] = 0;

	ret = drmModeAddFB2(drm->fd, buffer->w, buffer->h,
	                    format ? format : gbm_bo_get_format(bo),
	                    handles, strides, offsets, &buffer->id , 0);

	if (ret != 0) {
		ret = drmModeAddFB(drm->fd, buffer->w, buffer->h, 24, 32,
		                   buffer->stride, buffer->handle, &buffer->id);
		PEPPER_CHECK(ret, return PEPPER_FALSE, "drmModeAddFB() failed.\n");
	}

	buffer->bo = bo;
	gbm_bo_set_user_data(bo, buffer, NULL);

	return PEPPER_TRUE;
}

drm_buffer_t *
drm_buffer_create_gbm(pepper_drm_t *drm, struct gbm_surface *surface,
                      struct gbm_bo *bo)
{
	drm_buffer_t   *buffer;

	buffer = calloc(1, sizeof(drm_buffer_t));
	PEPPER_CHECK(buffer, return NULL, "calloc() failed.\n");

	if (!init_buffer_gbm(buffer, drm, bo, 0)) {
		free(buffer);
		return NULL;
	}

	buffer->type = DRM_BUFFER_TYPE_GBM;
	buffer->surface = surface;
	buffer->bo = bo;

	return buffer;
}

static void
handle_client_buffer_destroy(pepper_event_listener_t *listener,
                             pepper_object_t *object,
                             uint32_t id, void *info, void *data)
{
	drm_buffer_t *buffer = data;
	buffer->client_buffer = NULL;
}

drm_buffer_t *
drm_buffer_create_client(pepper_drm_t *drm,
                         struct gbm_bo *bo, pepper_buffer_t *client_buffer, uint32_t format)
{
	drm_buffer_t *buffer;

	buffer = calloc(1, sizeof(drm_buffer_t));
	PEPPER_CHECK(buffer, return NULL, "calloc() failed.\n");

	if (!init_buffer_gbm(buffer, drm, bo, format)) {
		free(buffer);
		return NULL;
	}

	buffer->type = DRM_BUFFER_TYPE_CLIENT;
	buffer->client_buffer = client_buffer;
	pepper_buffer_reference(client_buffer);
	buffer->client_buffer_destroy_listener =
	        pepper_object_add_event_listener((pepper_object_t *)client_buffer,
	                        PEPPER_EVENT_OBJECT_DESTROY, 0,
	                        handle_client_buffer_destroy, buffer);

	return buffer;
}

void
drm_buffer_release(drm_buffer_t *buffer)
{
	if (buffer->type == DRM_BUFFER_TYPE_GBM)
		gbm_surface_release_buffer(buffer->surface, buffer->bo);
	else if (buffer->type == DRM_BUFFER_TYPE_CLIENT)
		drm_buffer_destroy(buffer);
}

void
drm_buffer_destroy(drm_buffer_t *buffer)
{
	drmModeRmFB(buffer->drm->fd, buffer->id);

	if (buffer->type == DRM_BUFFER_TYPE_DUMB) {
		struct drm_mode_destroy_dumb destroy_arg;

		if (buffer->image)
			pixman_image_unref(buffer->image);

		munmap(buffer->map, buffer->size);

		memset(&destroy_arg, 0x00, sizeof(destroy_arg));
		destroy_arg.handle = buffer->handle;
		drmIoctl(buffer->drm->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg);
	} else if (buffer->type == DRM_BUFFER_TYPE_GBM) {
		gbm_bo_set_user_data(buffer->bo, NULL, NULL);
		gbm_surface_release_buffer(buffer->surface, buffer->bo);
	} else if (buffer->type == DRM_BUFFER_TYPE_CLIENT) {
		if (buffer->client_buffer) {
			pepper_buffer_unreference(buffer->client_buffer);
			pepper_event_listener_remove(buffer->client_buffer_destroy_listener);
		}

		gbm_bo_destroy(buffer->bo);
	}

	free(buffer);
}
