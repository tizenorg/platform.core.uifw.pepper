#include "pepper-internal.h"
#include <string.h>

static void
view_mark_dirty(pepper_view_t *view, uint32_t flag)
{
    if (view->dirty & flag)
        return;

    view->dirty |= flag;

    if ((flag & PEPPER_VIEW_VISIBILITY_DIRTY) ||
        (flag & PEPPER_VIEW_GEOMETRY_DIRTY))
    {
        pepper_view_t *child;

        pepper_list_for_each(child, &view->children_list, parent_link)
            view_mark_dirty(child, flag);
    }

    pepper_compositor_schedule_repaint(view->compositor);
}

static void
view_mark_damaged(pepper_view_t *view)
{
    int i;

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
        view->plane_entries[i].need_damage = PEPPER_TRUE;
}

static void
view_damage_below(pepper_view_t *view)
{
    int i;

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
    {
        pepper_plane_entry_t *entry = &view->plane_entries[i];

        if (entry->plane)
            pepper_plane_add_damage_region(entry->plane, &entry->base.visible_region);
    }
}

void
pepper_view_surface_damage(pepper_view_t *view)
{
    int i;

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
    {
        pepper_plane_entry_t *entry = &view->plane_entries[i];

        if (entry->plane)
        {
            pixman_region32_t damage;

            pixman_region32_init(&damage);
            pixman_region32_copy(&damage, &view->surface->damage_region);
            pixman_region32_intersect_rect(&damage, &damage,
                                           0, 0, view->surface->w, view->surface->h);

            pepper_transform_pixman_region(&damage, &view->global_transform);
            pixman_region32_translate(&damage,
                                      -entry->plane->output->geometry.x,
                                      -entry->plane->output->geometry.y);
            pixman_region32_intersect(&damage, &damage, &entry->base.visible_region);
            pepper_plane_add_damage_region(entry->plane, &damage);
        }
    }
}

static void
view_handle_surface_destroy(pepper_event_listener_t *listener,
                            pepper_object_t *object, uint32_t id, void *info, void *data)
{
    pepper_view_t *view = data;
    PEPPER_ASSERT(view->surface != NULL);
    pepper_view_destroy(view);
}

static pepper_list_t *
view_insert(pepper_view_t *view, pepper_list_t *pos, pepper_bool_t subtree)
{
    if (pos->next != &view->compositor_link)
    {
        pepper_list_remove(&view->compositor_link);
        pepper_list_insert(pos, &view->compositor_link);
        view_mark_dirty(view, PEPPER_VIEW_Z_ORDER_DIRTY);
        pepper_object_emit_event(&view->base, PEPPER_EVENT_VIEW_STACK_CHANGE, NULL);
    }

    pos = &view->compositor_link;

    if (subtree)
    {
        pepper_view_t *child;

        pepper_list_for_each(child, &view->children_list, parent_link)
            pos = view_insert(child, pos, subtree);
    }

    return pos;
}

static void
plane_entry_handle_plane_destroy(pepper_event_listener_t *listener,
                                 pepper_object_t *object, uint32_t id, void *info, void *data);

static void
plane_entry_set_plane(pepper_plane_entry_t *entry, pepper_plane_t *plane)
{
    if (entry->plane == plane)
        return;

    if (entry->plane)
    {
        view_damage_below((pepper_view_t *)entry->base.view);
        entry->plane = NULL;
        pepper_event_listener_remove(entry->plane_destroy_listener);
        pixman_region32_fini(&entry->base.visible_region);
    }

    entry->plane = plane;

    if (entry->plane)
    {
        entry->plane_destroy_listener =
            pepper_object_add_event_listener(&plane->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                             plane_entry_handle_plane_destroy, entry);

        pixman_region32_init(&entry->base.visible_region);
        entry->need_damage = PEPPER_TRUE;
    }
}

static void
plane_entry_handle_plane_destroy(pepper_event_listener_t *listener,
                                 pepper_object_t *object, uint32_t id, void *info, void *data)
{
    pepper_plane_entry_t *entry = data;
    PEPPER_ASSERT(entry->plane != NULL);
    plane_entry_set_plane(entry, NULL);
}

void
pepper_view_assign_plane(pepper_view_t *view, pepper_output_t *output, pepper_plane_t *plane)
{
    if (plane && plane->output != output)
    {
        PEPPER_ERROR("Output mismatch.\n");
        return;
    }

    plane_entry_set_plane(&view->plane_entries[output->id], plane);
}

static void
view_update_geometry(pepper_view_t *view)
{
    pepper_output_t *output;

    if (view->surface)
    {
        view->w = view->surface->w;
        view->h = view->surface->h;
    }

    pepper_mat4_init_translate(&view->global_transform, view->x, view->y, 0.0);
    pepper_mat4_multiply(&view->global_transform, &view->transform, &view->global_transform);

    if (view->parent)
    {
        pepper_mat4_multiply(&view->global_transform,
                             &view->parent->global_transform, &view->global_transform);
    }

    /* Bounding region. */
    pixman_region32_init_rect(&view->bounding_region, 0, 0, view->w, view->h);
    pepper_transform_pixman_region(&view->bounding_region, &view->global_transform);

    /* Opaque region. */
    pixman_region32_init(&view->opaque_region);

    if (view->surface && pepper_mat4_is_translation(&view->global_transform))
    {
        pixman_region32_copy(&view->opaque_region, &view->surface->opaque_region);
        pixman_region32_translate(&view->opaque_region,
                                  view->global_transform.m[3], view->global_transform.m[7]);
    }

    view->output_overlap = 0;

    pepper_list_for_each(output, &view->compositor->output_list, link)
    {
        pixman_box32_t   box =
        {
            output->geometry.x,
            output->geometry.y,
            output->geometry.x + output->geometry.w,
            output->geometry.y + output->geometry.h
        };

        if (pixman_region32_contains_rectangle(&view->bounding_region, &box) != PIXMAN_REGION_OUT)
            view->output_overlap |= (1 << output->id);
    }

}

void
pepper_view_update(pepper_view_t *view)
{
    if (!view->dirty)
        return;

    if (view->parent)
    {
        pepper_view_update(view->parent);
        view->visible = view->parent->visible && view->mapped;
    }
    else
    {
        view->visible = view->mapped;
    }

    if (view->visible == view->prev_visible)
        view->dirty &= ~PEPPER_VIEW_VISIBILITY_DIRTY;

    if (!view->dirty)
        return;

    if (view->prev_visible)
        view_damage_below(view);

    if ((view->dirty & PEPPER_VIEW_GEOMETRY_DIRTY) || (!view->prev_visible && view->visible))
        view_update_geometry(view);

    if (view->visible)
        view_mark_damaged(view);

    view->dirty = 0;
    view->prev_visible = view->visible;
}

static void
view_init(pepper_view_t *view, pepper_compositor_t *compositor)
{
    int i;

    view->compositor_link.item = view;
    view->parent_link.item = view;
    view->link.item = view;
    view->surface_link.item = view;

    view->compositor = compositor;
    pepper_list_insert(compositor->view_list.prev, &view->compositor_link);

    pepper_list_init(&view->children_list);

    pepper_mat4_init_identity(&view->transform);
    pepper_mat4_init_identity(&view->global_transform);
    pixman_region32_init(&view->bounding_region);
    pixman_region32_init(&view->opaque_region);

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
    {
        view->plane_entries[i].base.view = view;
        view->plane_entries[i].link.item = &view->plane_entries[i];
    }
}

PEPPER_API pepper_view_t *
pepper_compositor_add_surface_view(pepper_compositor_t *compositor, pepper_surface_t *surface)
{
    pepper_view_t *view = (pepper_view_t *)pepper_object_alloc(PEPPER_OBJECT_VIEW,
                                                               sizeof(pepper_view_t));
    if (!view)
    {
        PEPPER_ERROR("Failed to allocate a pepper object.\n");
        return NULL;
    }

    view_init(view, compositor);

    view->x = 0.0;
    view->y = 0.0;
    view->w = surface->w;
    view->h = surface->h;

    view->surface = surface;
    pepper_list_insert(&surface->view_list, &view->surface_link);

    view->surface_destroy_listener =
        pepper_object_add_event_listener(&surface->base, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                         view_handle_surface_destroy, view);

    return view;
}

PEPPER_API void
pepper_view_destroy(pepper_view_t *view)
{
    int             i;
    pepper_view_t  *child, *tmp;

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
        plane_entry_set_plane(&view->plane_entries[i], NULL);

    pepper_list_for_each_safe(child, tmp, &view->children_list, parent_link)
        pepper_view_destroy(child);

    if (view->parent)
        pepper_list_remove(&view->parent_link);

    pepper_list_remove(&view->compositor_link);

    if (view->surface)
    {
        pepper_list_remove(&view->surface_link);
        pepper_event_listener_remove(view->surface_destroy_listener);
    }

    pixman_region32_fini(&view->opaque_region);
    pixman_region32_fini(&view->bounding_region);

    pepper_object_fini(&view->base);
    pepper_free(view);
}

PEPPER_API pepper_compositor_t *
pepper_view_get_compositor(pepper_view_t *view)
{
    return view->compositor;
}

PEPPER_API pepper_surface_t *
pepper_view_get_surface(pepper_view_t *view)
{
    return view->surface;
}

PEPPER_API void
pepper_view_set_parent(pepper_view_t *view, pepper_view_t *parent)
{
    if (view->parent == parent)
        return;

    if (view->parent)
        pepper_list_remove(&view->parent_link);

    view->parent = parent;

    if (view->parent)
        pepper_list_insert(view->parent->children_list.prev, &view->parent_link);

    view_mark_dirty(view, PEPPER_VIEW_VISIBILITY_DIRTY | PEPPER_VIEW_GEOMETRY_DIRTY);
}

PEPPER_API pepper_view_t *
pepper_view_get_parent(pepper_view_t *view)
{
    return view->parent;
}

PEPPER_API pepper_bool_t
pepper_view_stack_above(pepper_view_t *view, pepper_view_t *below, pepper_bool_t subtree)
{
    view_insert(view, &below->compositor_link, subtree);
    return PEPPER_TRUE;
}

PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_view_t *view, pepper_view_t *above, pepper_bool_t subtree)
{
    view_insert(view, above->compositor_link.prev, subtree);
    return PEPPER_TRUE;
}

PEPPER_API void
pepper_view_stack_top(pepper_view_t *view, pepper_bool_t subtree)
{
    view_insert(view, view->compositor->view_list.prev, subtree);
}

PEPPER_API void
pepper_view_stack_bottom(pepper_view_t *view, pepper_bool_t subtree)
{
    view_insert(view, &view->compositor->view_list, subtree);
}

PEPPER_API pepper_view_t *
pepper_view_get_above(pepper_view_t *view)
{
    return view->compositor_link.next->item;
}

PEPPER_API pepper_view_t *
pepper_view_get_below(pepper_view_t *view)
{
    return view->compositor_link.prev->item;
}

PEPPER_API const pepper_list_t *
pepper_view_get_children_list(pepper_view_t *view)
{
    return &view->children_list;
}

PEPPER_API void
pepper_view_resize(pepper_view_t *view, int w, int h)
{
    if (view->w == w && view->h == h)
        return;

    view->w = w;
    view->h = h;
    view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
}

PEPPER_API void
pepper_view_get_size(pepper_view_t *view, int *w, int *h)
{
    if (w)
        *w = view->w;

    if (h)
        *h = view->h;
}

PEPPER_API void
pepper_view_set_position(pepper_view_t *view, double x, double y)
{
    if (view->x == x && view->y == y)
        return;

    view->x = x;
    view->y = y;
    view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
}

PEPPER_API void
pepper_view_get_position(pepper_view_t *view, double *x, double *y)
{
    if (x)
        *x = view->x;

    if (y)
        *y = view->y;
}

PEPPER_API void
pepper_view_set_transform(pepper_view_t *view, const pepper_mat4_t *matrix)
{
    pepper_mat4_copy(&view->transform, matrix);
    view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
}

PEPPER_API const pepper_mat4_t *
pepper_view_get_transform(pepper_view_t *view)
{
    return &view->transform;
}

PEPPER_API void
pepper_view_map(pepper_view_t *view)
{
    if (view->mapped)
        return;

    view->mapped = PEPPER_TRUE;
    view_mark_dirty(view, PEPPER_VIEW_VISIBILITY_DIRTY);
}

PEPPER_API void
pepper_view_unmap(pepper_view_t *view)
{
    if (!view->mapped)
        return;

    view->mapped = PEPPER_FALSE;
    view_mark_dirty(view, PEPPER_VIEW_VISIBILITY_DIRTY);
}

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_view_t *view)
{
    return view->mapped;
}

PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_view_t *view)
{
    return view->visible;
}
