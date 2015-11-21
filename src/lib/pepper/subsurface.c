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
    wl_resource_destroy(resource);
}

static void
subsurface_set_position(struct wl_client    *client,
                        struct wl_resource  *resource,
                        int32_t              x,
                        int32_t              y)
{
    pepper_subsurface_t *subsurface = wl_resource_get_user_data(resource);

    subsurface->pending.x = x;
    subsurface->pending.y = y;
}

static pepper_bool_t
subsurface_is_sibling(pepper_subsurface_t *subsurface, pepper_surface_t *sib)
{
    pepper_subsurface_t *sibling = sib->sub;

    if (sibling)
    {
        if (subsurface->parent == sibling->surface)
            return PEPPER_TRUE;
        else if(subsurface->parent == sibling->parent)
            return PEPPER_TRUE;
    }

    return PEPPER_FALSE;
}

static void
subsurface_stack_above(pepper_subsurface_t *subsurface, pepper_surface_t *sib)
{
    pepper_subsurface_t *sibling = sib->sub;

    /* TODO: sibling == parent */

    pepper_list_remove(&subsurface->pending.parent_link);
    pepper_list_insert(&sibling->pending.parent_link, &subsurface->pending.parent_link);

    subsurface->restacked = PEPPER_TRUE;
}

static void
subsurface_place_above(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *sibling_resource)
{
    pepper_subsurface_t *sub = wl_resource_get_user_data(resource);
    pepper_surface_t    *sibling;

    if (!sibling_resource)
    {
        wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
                               "reference surface cannot be null");
        return ;
    }

    sibling = wl_resource_get_user_data(sibling_resource);

    if (sub->surface == sibling)
    {
        wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
                               "cannot place above of its own for itself");
        return ;
    }

    if (!subsurface_is_sibling(sub, sibling))
    {
        wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
                               "reference surface is not sibling");
        return ;
    }

    subsurface_stack_above(sub, sibling);
}

static void
subsurface_stack_below(pepper_subsurface_t *subsurface, pepper_surface_t *sib)
{
    pepper_subsurface_t *sibling = sib->sub;

    /* TODO: sibling == parent */

    pepper_list_remove(&subsurface->pending.parent_link);
    pepper_list_insert(sibling->pending.parent_link.prev, &subsurface->pending.parent_link);

    subsurface->restacked = PEPPER_TRUE;
}

static void
subsurface_place_below(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *sibling_resource)
{
    pepper_subsurface_t *sub = wl_resource_get_user_data(resource);
    pepper_surface_t    *sibling;

    if (!sibling_resource)
    {
        wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
                               "reference surface cannot be null");
        return ;
    }

    sibling = wl_resource_get_user_data(sibling_resource);

    if (sub->surface == sibling)
    {
        wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
                               "cannot place below of its own for itself");
        return ;
    }

    if (!subsurface_is_sibling(sub, sibling))
    {
        wl_resource_post_error(resource, WL_SUBSURFACE_ERROR_BAD_SURFACE,
                               "reference surface is not sibling");
        return ;
    }

    subsurface_stack_below(sub, sibling);
}

static void
subsurface_set_sync(struct wl_client    *client,
                    struct wl_resource  *resource)
{
    pepper_subsurface_t *subsurface = wl_resource_get_user_data(resource);

    subsurface->synchronized = 1;
}

static void
subsurface_set_desync(struct wl_client      *client,
                      struct wl_resource    *resource)
{
    pepper_subsurface_t *subsurface = wl_resource_get_user_data(resource);

    if (subsurface->synchronized )
    {
        /* TODO: subsurface_commit(subsurface);? */
    }

    subsurface->synchronized = 0;
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
    pepper_view_t   *view;

    pepper_surface_state_fini(&subsurface->cache);
    pepper_list_remove(&subsurface->parent_link);
    pepper_list_remove(&subsurface->pending.parent_link);

    pepper_list_for_each(view, &subsurface->surface->view_list, surface_link)
        pepper_view_destroy(view);

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

static pepper_bool_t
pepper_subsurface_create_views(pepper_subsurface_t *subsurface)
{
    pepper_surface_t    *parent = subsurface->parent;
    pepper_view_t       *parent_view;

    pepper_list_for_each(parent_view, &parent->view_list, surface_link)
    {
        pepper_view_t *subview = pepper_compositor_add_view(parent->compositor);
        PEPPER_CHECK(subview, return PEPPER_FALSE, "pepper_compositor_add_view() failed.\n");

        pepper_view_set_surface(subview, subsurface->surface);
        pepper_view_set_parent(subview, parent_view);
        pepper_view_set_transform_inherit(subview, PEPPER_TRUE);
        pepper_view_map(subview);
    }

    return PEPPER_TRUE;
}

pepper_subsurface_t *
pepper_subsurface_create(pepper_surface_t   *surface,
                         pepper_surface_t   *parent,
                         struct wl_client   *client,
                         struct wl_resource *resource,
                         uint32_t            id)
{
    pepper_subsurface_t *subsurface = NULL;
    pepper_bool_t        ret;

    if (!pepper_surface_set_role(surface, "wl_subsurface"))
    {
        wl_resource_post_error(resource, WL_SUBCOMPOSITOR_ERROR_BAD_SURFACE,
                               "cannot assign wl_subsurface role");
        return NULL;
    }

    /* Make sure that subsurface has no view */
    if (!pepper_list_empty(&surface->view_list))
        goto error;

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

    /* create views that corresponding to parent's views */
    ret = pepper_subsurface_create_views(subsurface);
    PEPPER_CHECK(ret, goto error, "pepper_subsurface_create_views() failed\n");

    surface->sub = subsurface;

    return subsurface;

error:
    if (subsurface)
        pepper_subsurface_destroy(subsurface);

    return NULL;
}
