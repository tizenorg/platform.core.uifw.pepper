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
subsurface_destroy(struct wl_client     *client,
                   struct wl_resource   *resource)
{
    /* TODO */
}

static void
subsurface_set_position(struct wl_client    *client,
                        struct wl_resource  *resource,
                        int32_t              x,
                        int32_t              y)
{
    /* TODO */
}

static void
subsurface_place_above(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *sibling_resource)
{
    /* TODO */
}

static void
subsurface_place_below(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *sibling)
{
    /* TODO */
}

static void
subsurface_set_sync(struct wl_client    *client,
                    struct wl_resource  *resource)
{
    /* TODO */
}

static void
subsurface_set_desync(struct wl_client      *client,
                      struct wl_resource    *resource)
{
    /* TODO */
}

static struct wl_subsurface_interface subsurface_implementation =
{
    subsurface_destroy,
    subsurface_set_position,
    subsurface_place_above,
    subsurface_place_below,
    subsurface_set_sync,
    subsurface_set_desync,
};

void
pepper_subsurface_destroy(pepper_subsurface_t *subsurface)
{
    /* TODO */

    pepper_surface_state_fini(&subsurface->cache);
    pepper_list_remove(&subsurface->parent_link);
    pepper_list_remove(&subsurface->pending.parent_link);

    free(subsurface);
}

static void
subsurface_resource_destroy_handler(struct wl_resource *resource)
{
    pepper_subsurface_t *sub = wl_resource_get_user_data(resource);
    pepper_subsurface_destroy(sub);
}

static void
handle_parent_destroy(pepper_event_listener_t *listener,
                      pepper_object_t *object, uint32_t id, void *info, void *data)
{
    /* TODO */
}

pepper_subsurface_t *
pepper_subsurface_create(pepper_surface_t   *surface,
                         pepper_surface_t   *parent,
                         struct wl_client   *client,
                         struct wl_resource *resource,
                         uint32_t            id)
{
    pepper_subsurface_t *subsurface = NULL;

    if (!pepper_surface_set_role(surface, "wl_subsurface"))
    {
        wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
                               "cannot assign wl_subsurface role");
        return NULL;
    }

    subsurface = calloc(1, sizeof(pepper_subsurface_t));
    PEPPER_CHECK(subsurface, goto error, "calloc() failed.\n");

    subsurface->resource = wl_resource_create(client, &wl_subsurface_interface,
                                           wl_resource_get_version(resource), id);
    PEPPER_CHECK(subsurface->resource, goto error, "wl_resource_create() failed\n");

    wl_resource_set_implementation(subsurface->resource, &subsurface_implementation, subsurface,
                                   subsurface_resource_destroy_handler);

    subsurface->surface      = surface;
    subsurface->parent       = parent;
    subsurface->synchronized = PEPPER_TRUE;

    subsurface->x            = subsurface->y         = 0.f;
    subsurface->pending.x    = subsurface->pending.y = 0.f;

    pepper_surface_state_init(&subsurface->cache);

    pepper_object_add_event_listener(&parent->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                     handle_parent_destroy, subsurface);

    /* subsurface_list is z-order sorted, youngest child is top-most */
    pepper_list_insert(&parent->subsurface_list, &subsurface->parent_link);
    pepper_list_insert(&parent->subsurface_pending_list, &subsurface->pending.parent_link);

    surface->sub = subsurface;

    return subsurface;

error:
    if (subsurface)
        pepper_subsurface_destroy(subsurface);

    return NULL;
}
