#include "pepper-internal.h"
#include <string.h>

void
pepper_view_mark_dirty(pepper_view_t *view, uint32_t flag)
{
    pepper_view_t *child;

    if (view->dirty & flag)
        return;

    view->dirty |= flag;

    /* Mark entire subtree's geometry as dirty. */
    if (flag & PEPPER_VIEW_GEOMETRY_DIRTY)
    {
        pepper_list_for_each(child, &view->children_list, parent_link)
            pepper_view_mark_dirty(child, PEPPER_VIEW_GEOMETRY_DIRTY);
    }

    /* Mark entire subtree's active as dirty. */
    if (flag & PEPPER_VIEW_ACTIVE_DIRTY)
    {
        pepper_list_for_each(child, &view->children_list, parent_link)
            pepper_view_mark_dirty(child, PEPPER_VIEW_ACTIVE_DIRTY);
    }

    pepper_compositor_schedule_repaint(view->compositor);
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
            pixman_region32_intersect_rect(&damage, &damage, 0, 0, view->w, view->h);

            pepper_transform_pixman_region(&damage, &view->global_transform);
            pixman_region32_translate(&damage,
                                      -entry->plane->output->geometry.x,
                                      -entry->plane->output->geometry.y);
            pixman_region32_intersect(&damage, &damage, &entry->base.visible_region);
            pepper_plane_add_damage_region(entry->plane, &damage);
        }
    }
}

void
pepper_view_get_local_coordinate(pepper_view_t *view,
                                 double global_x, double global_y,
                                 double *local_x, double *local_y)
{
    pepper_vec4_t pos = { global_x, global_y, 0.0, 1.0 };

    pepper_mat4_transform_vec4(&view->global_transform_inverse, &pos);

    PEPPER_ASSERT(pos.w >= 1e-6);

    *local_x = pos.x / pos.w;
    *local_y = pos.y / pos.w;
}

void
pepper_view_get_global_coordinate(pepper_view_t *view,
                                  double local_x, double local_y,
                                  double *global_x, double *global_y)
{
    pepper_vec4_t pos = { local_x, local_y, 0.0, 1.0 };

    pepper_mat4_transform_vec4(&view->global_transform, &pos);

    PEPPER_ASSERT(pos.w >= 1e-6);

    *global_x = pos.x / pos.w;
    *global_y = pos.y / pos.w;
}

static pepper_list_t *
view_insert(pepper_view_t *view, pepper_list_t *pos, pepper_bool_t subtree)
{
    if (pos->next != &view->compositor_link)
    {
        pepper_list_remove(&view->compositor_link);
        pepper_list_insert(pos, &view->compositor_link);
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
plane_entry_set_plane(pepper_plane_entry_t *entry, pepper_plane_t *plane)
{
    if (entry->plane == plane)
        return;

    if (entry->plane)
    {
        pepper_plane_add_damage_region(entry->plane, &entry->base.visible_region);
        entry->plane = NULL;
        pixman_region32_fini(&entry->base.visible_region);
    }

    entry->plane = plane;

    if (entry->plane)
    {
        pixman_region32_init(&entry->base.visible_region);
        entry->need_damage = PEPPER_TRUE;
    }
}

void
pepper_view_assign_plane(pepper_view_t *view, pepper_output_t *output, pepper_plane_t *plane)
{
    PEPPER_CHECK(!plane || plane->output == output, return, "Plane output mismatch.\n");
    plane_entry_set_plane(&view->plane_entries[output->id], plane);
}

void
pepper_view_update(pepper_view_t *view)
{
    pepper_bool_t   active;
    int             i;

    if (!view->dirty)
        return;

    /* Update parent view first as transform and active flag are affected by the parent. */
    if (view->parent)
    {
        pepper_view_update(view->parent);
        active = view->parent->active && view->mapped;
    }
    else
    {
        active = view->mapped;
    }

    if (view->active == active)
        view->dirty &= ~PEPPER_VIEW_ACTIVE_DIRTY;

    if (!view->dirty)
        return;

    view->active = active;

    /* Damage for the view unmap will be handled by assigning NULL plane. */
    if (!view->active)
        return;

    /* We treat the modification as unmapping and remapping the view. So,
     * damage for the unmap and damage for the remap.
     *
     * Here, we know on which planes the view was previously located. So, we can
     * inflict damage on the planes for the unmap.
     *
     * However, new visible region of the view is not known at the moment
     * because no plane is assigned yet. So, simply mark the all plane entries
     * as damaged and the damage for the remap will be inflicted separately for
     * each output when the visible region is calculated on output repaint.
     */

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
    {
        pepper_plane_entry_t *entry = &view->plane_entries[i];

        if (entry->plane)
            pepper_plane_add_damage_region(entry->plane, &entry->base.visible_region);
    }

    /* Update geometry. */
    if (view->dirty & PEPPER_VIEW_GEOMETRY_DIRTY)
    {
        pepper_output_t *output;

        /* Transform. */
        pepper_mat4_init_translate(&view->global_transform, view->x, view->y, 0.0);
        pepper_mat4_multiply(&view->global_transform, &view->global_transform, &view->transform);

        if (view->parent)
        {
            pepper_mat4_multiply(&view->global_transform,
                                 &view->parent->global_transform, &view->global_transform);
        }

        pepper_mat4_inverse(&view->global_transform_inverse, &view->global_transform);

        /* Bounding region. */
        pixman_region32_fini(&view->bounding_region);
        pixman_region32_init_rect(&view->bounding_region, 0, 0, view->w, view->h);
        pepper_transform_pixman_region(&view->bounding_region, &view->global_transform);

        /* Opaque region. */
        if (view->surface && pepper_mat4_is_translation(&view->global_transform))
        {
            pixman_region32_copy(&view->opaque_region, &view->surface->opaque_region);
            pixman_region32_translate(&view->opaque_region,
                                      view->global_transform.m[12], view->global_transform.m[13]);
        }
        else
        {
            pixman_region32_clear(&view->opaque_region);
        }

        /* Output overlap. */
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

    /* Mark the plane entries as damaged. */
    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
        view->plane_entries[i].need_damage = PEPPER_TRUE;

    view->active = active;
    view->dirty = 0;
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
pepper_compositor_add_view(pepper_compositor_t *compositor)
{
    pepper_view_t *view = (pepper_view_t *)pepper_object_alloc(PEPPER_OBJECT_VIEW,
                                                               sizeof(pepper_view_t));
    PEPPER_CHECK(view, return NULL, "pepper_object_alloc() failed.\n");

    view_init(view, compositor);

    view->x = 0.0;
    view->y = 0.0;
    view->w = 0;
    view->h = 0;

    pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_VIEW_ADD, view);
    return view;
}

PEPPER_API pepper_bool_t
pepper_view_set_surface(pepper_view_t *view, pepper_surface_t *surface)
{
    if (view->surface == surface)
        return PEPPER_TRUE;

    if (view->surface)
        pepper_list_remove(&view->surface_link);

    view->surface = surface;

    if (view->surface)
        pepper_list_insert(&surface->view_list, &view->surface_link);

    pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
    return PEPPER_TRUE;
}

PEPPER_API void
pepper_view_destroy(pepper_view_t *view)
{
    int             i;
    pepper_view_t  *child, *tmp;

    pepper_object_emit_event(&view->compositor->base, PEPPER_EVENT_COMPOSITOR_VIEW_REMOVE, view);
    pepper_object_fini(&view->base);

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
        plane_entry_set_plane(&view->plane_entries[i], NULL);

    pepper_list_for_each_safe(child, tmp, &view->children_list, parent_link)
        pepper_view_destroy(child);

    if (view->parent)
        pepper_list_remove(&view->parent_link);

    pepper_list_remove(&view->compositor_link);

    if (view->surface)
        pepper_list_remove(&view->surface_link);

    pixman_region32_fini(&view->opaque_region);
    pixman_region32_fini(&view->bounding_region);

    free(view);
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

    pepper_view_mark_dirty(view, PEPPER_VIEW_ACTIVE_DIRTY | PEPPER_VIEW_GEOMETRY_DIRTY);
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
    pepper_view_mark_dirty(view, PEPPER_VIEW_Z_ORDER_DIRTY);
    return PEPPER_TRUE;
}

PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_view_t *view, pepper_view_t *above, pepper_bool_t subtree)
{
    view_insert(view, above->compositor_link.prev, subtree);
    pepper_view_mark_dirty(view, PEPPER_VIEW_Z_ORDER_DIRTY);
    return PEPPER_TRUE;
}

PEPPER_API void
pepper_view_stack_top(pepper_view_t *view, pepper_bool_t subtree)
{
    view_insert(view, view->compositor->view_list.prev, subtree);
    pepper_view_mark_dirty(view, PEPPER_VIEW_Z_ORDER_DIRTY);
}

PEPPER_API void
pepper_view_stack_bottom(pepper_view_t *view, pepper_bool_t subtree)
{
    view_insert(view, &view->compositor->view_list, subtree);
    pepper_view_mark_dirty(view, PEPPER_VIEW_Z_ORDER_DIRTY);
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
    pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
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
    pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
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
    pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
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
    pepper_view_mark_dirty(view, PEPPER_VIEW_ACTIVE_DIRTY);
}

PEPPER_API void
pepper_view_unmap(pepper_view_t *view)
{
    if (!view->mapped)
        return;

    view->mapped = PEPPER_FALSE;
    pepper_view_mark_dirty(view, PEPPER_VIEW_ACTIVE_DIRTY);
}

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_view_t *view)
{
    return view->mapped;
}

PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_view_t *view)
{
    if (view->parent)
        return pepper_view_is_visible(view->parent) && view->mapped;

    return view->mapped;
}

PEPPER_API pepper_bool_t
pepper_view_is_opaque(pepper_view_t *view)
{
    pepper_surface_t       *surface = view->surface;
    struct wl_shm_buffer   *shm_buffer = wl_shm_buffer_get(surface->buffer.buffer->resource);
    pixman_box32_t          extent;

    if (shm_buffer)
    {
        uint32_t shm_format = wl_shm_buffer_get_format(shm_buffer);

        if (shm_format == WL_SHM_FORMAT_XRGB8888 || shm_format == WL_SHM_FORMAT_RGB565)
            return PEPPER_TRUE;
    }

    /* TODO: format check for wl_drm or wl_tbm?? */

    extent.x1 = 0;
    extent.y1 = 0;
    extent.x2 = view->surface->w;
    extent.y2 = view->surface->h;

    if (pixman_region32_contains_rectangle(&surface->opaque_region, &extent) == PIXMAN_REGION_IN)
        return PEPPER_TRUE;

    return PEPPER_FALSE;
}
