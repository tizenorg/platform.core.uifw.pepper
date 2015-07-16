#include "pepper-internal.h"
#include <string.h>

static void
view_mark_plane_entries_damaged(pepper_view_t *view)
{
    int i;

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
        view->plane_entries[i].need_damage = PEPPER_TRUE;
}

static void
view_handle_surface_destroy(struct wl_listener *listener, void *data)
{
    pepper_view_t *view = pepper_container_of(listener, pepper_view_t, surface_destroy_listener);
    PEPPER_ASSERT(view->surface != NULL);
    pepper_view_destroy(view);
}

static void
view_geometry_dirty(pepper_view_t *view)
{
    pepper_list_t *l;

    if (view->geometry_dirty)
        return;

    view->geometry_dirty = PEPPER_TRUE;
    pepper_view_damage_below(view);

    PEPPER_LIST_FOR_EACH(&view->children_list, l)
        view_geometry_dirty((pepper_view_t *)l->item);
}

static void
view_update_visibility(pepper_view_t *view)
{
    pepper_bool_t visibility;

    if (view->parent)
        visibility = view->parent->visibility && view->mapped;
    else
        visibility = view->mapped;

    if (visibility != view->visibility)
    {
        pepper_list_t *l;

        view->visibility = visibility;

        /* We simply treat a visibility change as a geometry change. */
        view_geometry_dirty(view);

        PEPPER_LIST_FOR_EACH(&view->children_list, l)
            view_update_visibility((pepper_view_t *)l->item);
    }
}

static void
view_map(pepper_view_t *view)
{
    if (view->mapped)
        return;

    view->mapped = PEPPER_TRUE;
    view_update_visibility(view);
}

static void
view_unmap(pepper_view_t *view)
{
    if (!view->mapped)
        return;

    view->mapped = PEPPER_FALSE;
    view_update_visibility(view);
}

static inline void
add_bbox_point(double *box, int x, int y, const pepper_mat4_t *matrix)
{
    pepper_vec2_t v = { x, y };

    pepper_mat4_transform_vec2(matrix, &v);

    box[0] = PEPPER_MIN(box[0], v.x);
    box[1] = PEPPER_MIN(box[1], v.y);
    box[2] = PEPPER_MAX(box[2], v.x);
    box[3] = PEPPER_MAX(box[3], v.y);
}

static inline void
transform_bounding_box(pixman_box32_t *box, const pepper_mat4_t *matrix)
{
    double          b[4] = { HUGE_VAL, HUGE_VAL, -HUGE_VAL, -HUGE_VAL };

    add_bbox_point(b, box->x1, box->y1, matrix);
    add_bbox_point(b, box->x2, box->y1, matrix);
    add_bbox_point(b, box->x2, box->y2, matrix);
    add_bbox_point(b, box->x1, box->y2, matrix);

    box->x1 = floor(b[0]);
    box->y1 = floor(b[1]);
    box->x2 = ceil(b[2]);
    box->y2 = ceil(b[3]);
}

static inline void
transform_region_bounding(pixman_region32_t *region, const pepper_mat4_t *matrix)
{
    pixman_region32_t   result;
    pixman_box32_t     *rects;
    int                 i, num_rects;

    pixman_region32_init(&result);
    rects = pixman_region32_rectangles(region, &num_rects);

    for (i = 0; i < num_rects; i++)
    {
        pixman_box32_t box = rects[i];

        transform_bounding_box(&box, matrix);
        pixman_region32_union_rect(&result, &result,
                                   box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
    }

    pixman_region32_copy(region, &result);
    pixman_region32_fini(&result);
}

static void
view_update_output_overlap(pepper_view_t *view)
{
    pepper_list_t *l;

    view->output_overlap = 0;

    PEPPER_LIST_FOR_EACH(&view->compositor->output_list, l)
    {
        pepper_output_t *output = l->item;
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

static pepper_list_t *
view_insert(pepper_view_t *view, pepper_list_t *pos, pepper_bool_t subtree)
{
    if (pos->next != &view->compositor_link)
    {
        pepper_list_remove(&view->compositor_link, NULL);
        pepper_list_insert(pos, &view->compositor_link);

        if (view->visibility)
        {
            pepper_view_damage_below(view);
            view_mark_plane_entries_damaged(view);
        }
    }

    pos = &view->compositor_link;

    if (subtree)
    {
        pepper_list_t *l;

        PEPPER_LIST_FOR_EACH(&view->children_list, l)
            pos = view_insert((pepper_view_t *)l->item, pos, subtree);
    }

    return pos;
}
static void
view_handle_plane_destroy(struct wl_listener *listener, void *data);

static void
plane_entry_set_plane(pepper_plane_entry_t *entry, pepper_plane_t *plane)
{
    if (entry->plane == plane)
        return;

    if (entry->plane)
    {
        pepper_view_damage_below((pepper_view_t *)entry->base.view);
        entry->plane = NULL;
        wl_list_remove(&entry->plane_destroy_listener.link);
        pixman_region32_fini(&entry->base.visible_region);
    }

    entry->plane = plane;

    if (entry->plane)
    {
        entry->plane_destroy_listener.notify = view_handle_plane_destroy;
        pepper_object_add_destroy_listener(&plane->base, &entry->plane_destroy_listener);
        pixman_region32_init(&entry->base.visible_region);
        entry->need_damage = PEPPER_TRUE;
    }
}

static void
view_handle_plane_destroy(struct wl_listener *listener, void *data)
{
    pepper_plane_entry_t *entry =
        pepper_container_of(listener, pepper_plane_entry_t, plane_destroy_listener);

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

void
pepper_view_damage_below(pepper_view_t *view)
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
pepper_view_update_geometry(pepper_view_t *view)
{
    if (!view->geometry_dirty)
        return;

    pepper_mat4_init_translate(&view->global_transform, view->x, view->y, 0.0);
    pepper_mat4_multiply(&view->global_transform, &view->transform, &view->global_transform);

    if (view->parent)
    {
        pepper_view_update_geometry(view->parent);
        pepper_mat4_multiply(&view->global_transform,
                             &view->parent->global_transform, &view->global_transform);
    }

    if (view->surface)
    {
        view->w = view->surface->w;
        view->h = view->surface->h;
    }

    /* Bounding region. */
    pixman_region32_init_rect(&view->bounding_region, 0, 0, view->w, view->h);
    transform_region_bounding(&view->bounding_region, &view->global_transform);

    /* Opaque region. */
    pixman_region32_init(&view->opaque_region);

    if (view->surface && pepper_mat4_is_translation(&view->global_transform))
    {
        pixman_region32_copy(&view->opaque_region, &view->surface->opaque_region);
        pixman_region32_translate(&view->opaque_region,
                                  view->global_transform.m[3], view->global_transform.m[7]);
    }

    view->geometry_dirty = PEPPER_FALSE;
    view_update_output_overlap(view);
    view_mark_plane_entries_damaged(view);
}

static void
view_init(pepper_view_t *view, pepper_compositor_t *compositor)
{
    int i;

    view->compositor = compositor;
    view->compositor_link.item = view;
    pepper_list_insert(compositor->view_list.prev, &view->compositor_link);

    view->parent_link.item = view;
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
    pepper_view_t *view = (pepper_view_t *)pepper_object_alloc(sizeof(pepper_view_t));
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

    view->geometry_dirty = PEPPER_TRUE;

    view->surface = surface;
    view->surface_link.item = view;
    pepper_list_insert(&surface->view_list, &view->surface_link);
    view->surface_destroy_listener.notify = view_handle_surface_destroy;
    pepper_object_add_destroy_listener(&surface->base, &view->surface_destroy_listener);

    return view;
}

PEPPER_API void
pepper_view_destroy(pepper_view_t *view)
{
    pepper_list_t  *l, *next;
    int             i;

    /* Destroy signal is emitted in here so that any children that are willing to survive this
     * destruction can detach from their parent.
     */
    pepper_object_fini(&view->base);
    view_unmap(view);

    for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
        plane_entry_set_plane(&view->plane_entries[i], NULL);

    PEPPER_LIST_FOR_EACH_SAFE(&view->children_list, l, next)
        pepper_view_destroy((pepper_view_t *)(l->item));

    PEPPER_ASSERT(pepper_list_empty(&view->children_list));

    if (view->parent)
        pepper_list_remove(&view->parent_link, NULL);

    pepper_list_remove(&view->compositor_link, NULL);

    if (view->surface)
    {
        pepper_list_remove(&view->surface_link, NULL);
        wl_list_remove(&view->surface_destroy_listener.link);
    }

    pixman_region32_fini(&view->opaque_region);
    pixman_region32_fini(&view->bounding_region);

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

    view->parent = parent;
    pepper_list_remove(&view->parent_link, NULL);

    if (view->parent)
        pepper_list_insert(view->parent->children_list.prev, &view->parent_link);

    view_geometry_dirty(view);
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
    view_geometry_dirty(view);
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
    view_geometry_dirty(view);
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
    view_geometry_dirty(view);
}

PEPPER_API const pepper_mat4_t *
pepper_view_get_transform(pepper_view_t *view)
{
    return &view->transform;
}

PEPPER_API void
pepper_view_map(pepper_view_t *view)
{
    view_map(view);
}

PEPPER_API void
pepper_view_unmap(pepper_view_t *view)
{
    view_unmap(view);
}

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_view_t *view)
{
    return view->mapped;
}

PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_view_t *view)
{
    return view->visibility;
}
