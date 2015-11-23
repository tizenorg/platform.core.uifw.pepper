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
    if (subsurface->parent == sib)
        return PEPPER_TRUE;
    else if(sib->sub && sib->sub->parent == subsurface->parent)
        return PEPPER_TRUE;

    return PEPPER_FALSE;
}

static void
subsurface_stack_above(pepper_subsurface_t *subsurface, pepper_subsurface_t *sibling)
{
    pepper_subsurface_t *parent;
    pepper_list_t       *pos;

    if (subsurface->parent == sibling->surface)
    {
        parent = sibling;
        pos = &parent->pending.self_link;
    }
    else
    {
        parent = subsurface->parent->sub;
        pos = &sibling->pending.parent_link;
    }

    pepper_list_remove(&subsurface->pending.parent_link);
    pepper_list_insert(pos, &subsurface->pending.parent_link);

    parent->need_restack = PEPPER_TRUE;
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

    subsurface_stack_above(sub, sibling->sub);
}

static void
subsurface_stack_below(pepper_subsurface_t *subsurface, pepper_subsurface_t *sibling)
{
    pepper_subsurface_t *parent;
    pepper_list_t       *pos;

    if (subsurface->parent == sibling->surface)
    {
        parent = sibling;
        pos = &parent->pending.self_link;
    }
    else
    {
        parent = subsurface->parent->sub;
        pos = &sibling->pending.parent_link;
    }

    pepper_list_remove(&subsurface->pending.parent_link);
    pepper_list_insert(pos->prev, &subsurface->pending.parent_link);

    parent->need_restack = PEPPER_TRUE;
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

    subsurface_stack_below(sub, sibling->sub);
}

static void
subsurface_set_sync(struct wl_client    *client,
                    struct wl_resource  *resource)
{
    pepper_subsurface_t *subsurface = wl_resource_get_user_data(resource);

    subsurface->sync = PEPPER_TRUE;
}

/* Copy 'from' -> 'to' state and clear 'from' state */
static void
surface_state_move(pepper_surface_state_t *from, pepper_surface_state_t *to)
{
    if (from->newly_attached)
    {
        to->newly_attached = PEPPER_TRUE;
        to->buffer         = from->buffer;

        from->newly_attached = PEPPER_FALSE;
        from->buffer         = NULL;
    }

    to->transform = from->transform;
    to->scale     = from->scale;
    to->x        += from->x;
    to->y        += from->y;

    /* FIXME: Need to create another one? */
    to->buffer_destroy_listener = from->buffer_destroy_listener;

    pixman_region32_copy(&to->damage_region, &from->damage_region);
    pixman_region32_copy(&to->opaque_region, &from->opaque_region);
    pixman_region32_copy(&to->input_region,  &from->input_region);

    wl_list_insert_list(&to->frame_callback_list, &from->frame_callback_list);

    /* Clear 'from' state */
    from->x         = 0;
    from->y         = 0;
    from->scale     = 1;
    from->transform = WL_OUTPUT_TRANSFORM_NORMAL;
    from->buffer_destroy_listener = NULL;

    pixman_region32_clear(&from->damage_region);
    pixman_region32_clear(&from->opaque_region);
    pixman_region32_clear(&from->input_region);

    wl_list_init(&from->frame_callback_list);
}

static void
surface_commit_to_cache(pepper_subsurface_t *subsurface)
{
    /* Commit surface.pending to subsurface.cache */
    surface_state_move(&subsurface->surface->pending, &subsurface->cache);
    subsurface->cached = PEPPER_TRUE;
}

static void
surface_commit_from_cache(pepper_subsurface_t *subsurface)
{
    /* check if dummy */
    if (!subsurface->parent)
        return ;

    /* Commit subsurface.cache to surface.current directly */
    pepper_surface_commit_state(subsurface->surface, &subsurface->cache);
    subsurface->cached = PEPPER_FALSE;

    /* Subsurface emit commit event in here */
    pepper_object_emit_event(&subsurface->surface->base, PEPPER_EVENT_SURFACE_COMMIT, NULL);
}

static pepper_bool_t
subsurface_get_sync(pepper_subsurface_t *subsurface)
{
    /* TODO: FIXME */
    if (!subsurface)
        return PEPPER_FALSE;

    if (!subsurface->parent)
        return PEPPER_FALSE;

    if (subsurface->sync)
        return PEPPER_TRUE;

    if (subsurface->parent->sub)
        return subsurface_get_sync(subsurface->parent->sub);

    return PEPPER_FALSE;
}

#define EACH_LIST_FOR_EACH(pos1, head1, pos2, head2, member)                             \
    for (pos1 = pepper_container_of((head1)->next, pos1, member), pos2 = pepper_container_of((head2)->next, pos2, member);              \
         (&pos1->member != (head1)) && (&pos2->member != (head2));                                            \
         pos1 = pepper_container_of(pos1->member.next, pos1, member), pos2 = pepper_container_of(pos2->member.next, pos2, member))
static void
subsurface_restack_view(pepper_subsurface_t *subsurface)
{
    pepper_subsurface_t *child1 = NULL, *child2;
    pepper_list_t       *list;

    pepper_list_for_each_list(list, &subsurface->children_list)
    {
        pepper_view_t *view1, *view2;

        child2 = list->item;

        if (!child1)
        {
            child1 = child2;
            continue;
        }

        EACH_LIST_FOR_EACH(view1, &child1->surface->view_list,
                           view2, &child2->surface->view_list,
                           surface_link)
        {
            pepper_view_stack_above(view1, view2, PEPPER_TRUE);
        }

        child1 = child2;
    }
}
#undef EACH_LIST_FOR_EACH

static void
subsurface_apply_order(pepper_subsurface_t *subsurface)
{
    pepper_list_t       *list;
    pepper_subsurface_t *child;

    if (!subsurface->need_restack)
        return ;

    pepper_list_for_each_list(list, &subsurface->pending.children_list)
    {
        child = list->item;
        if (child)
        {
            /* */
            if (child == subsurface)
            {
                pepper_list_remove(&child->self_link);
                pepper_list_insert(&subsurface->children_list, &child->self_link);
            }
            else
            {
                pepper_list_remove(&child->parent_link);
                pepper_list_insert(&subsurface->children_list, &child->parent_link);
            }
        }
    }
    subsurface_restack_view(subsurface);

    subsurface->need_restack = PEPPER_FALSE;
}

static void
subsurface_apply_position(pepper_subsurface_t *subsurface)
{
    pepper_view_t *view;

    /* Check this subsurface is dummy */
    if (!subsurface->parent)
        return ;

    subsurface->x = subsurface->pending.x;
    subsurface->y = subsurface->pending.y;

    pepper_list_for_each(view, &subsurface->surface->view_list, surface_link)
        pepper_view_set_position(view, subsurface->x, subsurface->y);
}

static void
subsurface_apply_pending_state(pepper_subsurface_t *subsurface)
{
    subsurface_apply_position(subsurface);
    subsurface_apply_order(subsurface);
}

static void
subsurface_set_desync(struct wl_client      *client,
                      struct wl_resource    *resource)
{
    pepper_subsurface_t *subsurface = wl_resource_get_user_data(resource);

    if (subsurface->cached)
        surface_commit_from_cache(subsurface);

    subsurface->sync = PEPPER_FALSE;
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
    pepper_surface_state_fini(&subsurface->cache);

    if (subsurface->parent)
    {
        pepper_list_remove(&subsurface->parent_link);
        pepper_list_remove(&subsurface->pending.parent_link);
        pepper_event_listener_remove(subsurface->parent_destroy_listener);
        pepper_event_listener_remove(subsurface->parent_commit_listener);
    }

    /* TODO: handle view_list */

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
    pepper_subsurface_t *subsurface = data;

    /* TODO: handle view_list */

    pepper_list_remove(&subsurface->parent_link);
    pepper_list_remove(&subsurface->pending.parent_link);

    pepper_event_listener_remove(subsurface->parent_destroy_listener);

    subsurface->parent = NULL;
}

static void
subsurface_parent_commit(pepper_subsurface_t *subsurface)
{
    /* Apply subsurface's pending state, and propagate to children */
    subsurface_apply_pending_state(subsurface);

    if (subsurface->cached)
        surface_commit_from_cache(subsurface);
}

static void
handle_parent_commit(pepper_event_listener_t *listener,
                     pepper_object_t *object, uint32_t id, void *info, void *data)
{
    pepper_subsurface_t *subsurface = data;

    subsurface_parent_commit(subsurface);
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
        pepper_view_stack_above(subview, parent_view, PEPPER_TRUE);
        pepper_view_set_transform_inherit(subview, PEPPER_TRUE);

        /* FIXME: map later, when ? */
        pepper_view_map(subview);
    }

    return PEPPER_TRUE;
}

static pepper_subsurface_t *
pepper_dummy_subsurface_create_for_parent(pepper_surface_t *parent)
{
    pepper_subsurface_t *subsurface;

    subsurface = calloc(1, sizeof(pepper_subsurface_t));
    PEPPER_CHECK(subsurface, return NULL, "calloc() failed.\n");

    subsurface->surface = parent;

    /* Insert itself to own children_list. Parent can be placed below/above of own children */
    pepper_list_init(&subsurface->children_list);
    pepper_list_insert(&subsurface->children_list, &subsurface->self_link);

    pepper_list_init(&subsurface->pending.children_list);
    pepper_list_insert(&subsurface->pending.children_list, &subsurface->pending.self_link);

    return subsurface;
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

    /* A sub-surface is initially in the synchronized mode. */
    subsurface->sync         = PEPPER_TRUE;

    subsurface->x            = subsurface->y         = 0.f;
    subsurface->pending.x    = subsurface->pending.y = 0.f;

    subsurface->parent_link.item         = subsurface;
    subsurface->self_link.item           = subsurface;
    subsurface->pending.parent_link.item = subsurface;
    subsurface->pending.self_link.item   = subsurface;

    subsurface->parent_destroy_listener =
        pepper_object_add_event_listener(&parent->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                         handle_parent_destroy, subsurface);

    subsurface->parent_commit_listener =
        pepper_object_add_event_listener(&parent->base, PEPPER_EVENT_SURFACE_COMMIT, 0,
                                         handle_parent_commit, subsurface);

    pepper_surface_state_init(&subsurface->cache);

    if (!parent->sub)
        parent->sub = pepper_dummy_subsurface_create_for_parent(parent);
    PEPPER_CHECK(parent->sub, goto error, "pepper_dummy_subsurface_create_for_parent() failed\n");

    /* children_list is z-order sorted, youngest child is top-most */
    pepper_list_init(&subsurface->children_list);
    pepper_list_init(&subsurface->pending.children_list);

    /* link to myself */
    pepper_list_insert(&subsurface->children_list, &subsurface->self_link);
    pepper_list_insert(&subsurface->pending.children_list, &subsurface->pending.self_link);

    /* link to parent */
    pepper_list_insert(&parent->sub->children_list, &subsurface->parent_link);
    pepper_list_insert(&parent->sub->pending.children_list, &subsurface->pending.parent_link);

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

pepper_bool_t
pepper_subsurface_commit(pepper_subsurface_t *subsurface)
{
    if (subsurface_get_sync(subsurface))
    {
        surface_commit_to_cache(subsurface);

        /* consume this commit */
        return PEPPER_TRUE;
    }

    return PEPPER_FALSE;
}

void
subsurface_destroy_children_views(pepper_subsurface_t *subsurface, pepper_view_t *parent_view)
{
    pepper_list_t *list;

    if (!subsurface)
        return ;

    pepper_list_for_each_list(list, &subsurface->children_list)
    {
        pepper_subsurface_t *child = list->item;

        /* Except its own */
        if(child && (child != subsurface))
        {
            pepper_view_t *view;

            pepper_list_for_each(view, &subsurface->surface->view_list, surface_link)
            {
                if (view->parent == parent_view)
                {
                    /* FIXME: need this ? */
                    pepper_view_set_surface(view, NULL);
                    pepper_view_destroy(view);
                }
            }
        }
    }
}

void
subsurface_create_children_views(pepper_subsurface_t *subsurface, pepper_view_t *parent_view)
{
    pepper_list_t *list;

    if (!subsurface)
        return ;

    pepper_list_for_each_list(list, &subsurface->children_list)
    {
        pepper_subsurface_t *child = list->item;

        /* Except its own */
        if(child && (child != subsurface))
        {
            pepper_view_t *view = pepper_compositor_add_view(subsurface->surface->compositor);
            pepper_view_set_surface(view, child->surface);
            pepper_view_set_parent(view, parent_view);
            pepper_view_set_transform_inherit(view, PEPPER_TRUE);

            /* FIXME */
            pepper_view_map(view);
        }
    }

    subsurface->need_restack = PEPPER_TRUE;
}
