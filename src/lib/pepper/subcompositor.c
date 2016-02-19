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

static void
subcompositor_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void
subcompositor_get_subsurface(struct wl_client   *client,
                             struct wl_resource *resource,
                             uint32_t            id,
                             struct wl_resource *surface_resource,
                             struct wl_resource *parent_resource)
{
	pepper_surface_t        *surface = wl_resource_get_user_data(surface_resource);
	pepper_surface_t        *parent  = wl_resource_get_user_data(parent_resource);

	if (surface->sub) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
		                       "wl_subcompositor::get_subsurface() already requested");
		return ;
	}

	if (surface == parent) {
		wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
		                       "wl_subcompositor::get_subsurface() cannot assign parent for its own");
		return ;
	}

	if (!pepper_subsurface_create(surface, parent, client, resource, id))
		wl_resource_post_no_memory(resource);
}

static const struct wl_subcompositor_interface subcompositor_interface = {
	subcompositor_destroy,
	subcompositor_get_subsurface,
};

static void
unbind_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
subcompositor_bind(struct wl_client *client,
                   void             *data,
                   uint32_t          version,
                   uint32_t          id)
{
	pepper_subcompositor_t  *subcompositor = (pepper_subcompositor_t *)data;
	struct wl_resource      *resource;

	resource = wl_resource_create(client, &wl_subcompositor_interface, version, id);

	if (!resource) {
		PEPPER_ERROR("wl_resource_create failed\n");
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&subcompositor->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &subcompositor_interface,
	                               subcompositor, unbind_resource);
}

pepper_subcompositor_t *
pepper_subcompositor_create(pepper_compositor_t *compositor)
{
	pepper_subcompositor_t *subcompositor;

	subcompositor = (pepper_subcompositor_t *)pepper_object_alloc(
	                        PEPPER_OBJECT_SUBCOMPOSITOR,
	                        sizeof(pepper_subcompositor_t));
	PEPPER_CHECK(subcompositor, goto error, "pepper_object_alloc() failed.\n");

	subcompositor->compositor = compositor;
	subcompositor->global = wl_global_create(compositor->display,
	                        &wl_subcompositor_interface, 1,
	                        subcompositor, subcompositor_bind);
	PEPPER_CHECK(subcompositor->global, goto error, "wl_global_create() failed.\n");

	wl_list_init(&subcompositor->resource_list);

	return subcompositor;

error:
	if (subcompositor)
		pepper_subcompositor_destroy(subcompositor);

	return NULL;
}

void
pepper_subcompositor_destroy(pepper_subcompositor_t *subcompositor)
{
	struct wl_resource *resource, *tmp;

	if (subcompositor->global)
		wl_global_destroy(subcompositor->global);

	wl_resource_for_each_safe(resource, tmp, &subcompositor->resource_list)
	wl_resource_destroy(resource);

	pepper_object_fini(&subcompositor->base);
	free(subcompositor);
}
