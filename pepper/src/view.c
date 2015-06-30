#include "pepper-internal.h"
#include <string.h>

static void
view_handle_surface_destroy(struct wl_listener *listener, void *data)
{
    pepper_view_t *view = pepper_container_of(listener, pepper_view_t, surface_destroy_listener);
    PEPPER_ASSERT(view->surface != NULL);
    pepper_view_destroy(&view->base);
}

static void
view_geometry_dirty(pepper_view_t *view)
{
    pepper_list_t *l;

    if (view->geometry_dirty)
        return;

    view->geometry_dirty = PEPPER_TRUE;
    pepper_compositor_add_damage(view->compositor, &view->visible_region);

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

PEPPER_API pepper_object_t *
pepper_compositor_add_surface_view(pepper_object_t *comp, pepper_object_t *sfc)
{
    pepper_view_t       *view;
    pepper_compositor_t *compositor = (pepper_compositor_t *)comp;

    CHECK_MAGIC_AND_NON_NULL(comp, PEPPER_COMPOSITOR);
    CHECK_MAGIC_IF_NON_NULL(sfc, PEPPER_SURFACE);

    view = (pepper_view_t *)pepper_object_alloc(sizeof(pepper_view_t), PEPPER_VIEW);
    if (!view)
    {
        PEPPER_ERROR("Failed to allocate a pepper object.\n");
        return NULL;
    }

    view->compositor = compositor;

    view->x = 0.0;
    view->y = 0.0;

    view->w = 0;
    view->h = 0;

    pepper_mat4_init_identity(&view->transform);
    pepper_mat4_init_identity(&view->matrix_to_parent);
    pepper_mat4_init_identity(&view->matrix_to_global);

    view->parent_link.item = (void *)view;
    view->z_link.item = (void *)view;

    pepper_list_init(&view->children_list);
    pepper_list_insert(compositor->root_view_list.prev, &view->parent_link);
    pepper_list_insert(compositor->view_list.prev, &view->z_link);

    if (sfc)
    {
        pepper_surface_t *surface = (pepper_surface_t *)sfc;

        view->surface = surface;
        wl_list_insert(&surface->view_list, &view->surface_link);

        view->surface_destroy_listener.notify = view_handle_surface_destroy;
        pepper_object_add_destroy_listener(&surface->base, &view->surface_destroy_listener);

        view->w = surface->w;
        view->h = surface->h;
    }

    pixman_region32_init_rect(&view->bounding_region, 0, 0, view->w, view->h);
    pixman_region32_init(&view->opaque_region);
    pixman_region32_init(&view->visible_region);

    return &view->base;
}

PEPPER_API void
pepper_view_destroy(pepper_object_t *v)
{
    pepper_view_t  *view = (pepper_view_t *)v;
    pepper_list_t  *l, *next;

    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    /* Destroy signal is emitted in here so that any children that are willing to survive this
     * destruction can detach from their parent.
     */
    pepper_object_fini(&view->base);
    view_unmap(view);

    PEPPER_LIST_FOR_EACH_SAFE(&view->children_list, l, next)
        pepper_view_destroy((pepper_object_t *)(l->item));

    PEPPER_ASSERT(pepper_list_empty(&view->children_list));

    pepper_list_remove(&view->parent_link, NULL);
    pepper_list_remove(&view->z_link, NULL);

    if (view->surface)
    {
        wl_list_remove(&view->surface_link);
        wl_list_remove(&view->surface_destroy_listener.link);
    }

    pixman_region32_fini(&view->opaque_region);
    pixman_region32_fini(&view->visible_region);
    pixman_region32_fini(&view->bounding_region);

    pepper_free(view);
}

PEPPER_API pepper_object_t *
pepper_view_get_compositor(pepper_object_t *v)
{
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return &((pepper_view_t *)v)->compositor->base;
}

PEPPER_API pepper_object_t *
pepper_view_get_surface(pepper_object_t *v)
{
    pepper_view_t *view = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    if (view->surface)
        return &((pepper_view_t *)view)->surface->base;

    return NULL;
}

PEPPER_API void
pepper_view_set_parent(pepper_object_t *v, pepper_object_t *p)
{
    pepper_view_t *view = (pepper_view_t *)v;
    pepper_view_t *parent = (pepper_view_t *)p;

    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    CHECK_MAGIC_IF_NON_NULL(p, PEPPER_VIEW);

    if (view->parent == parent)
        return;

    view->parent = parent;
    pepper_list_remove(&view->parent_link, NULL);

    if (view->parent)
        pepper_list_insert(view->parent->children_list.prev, &view->parent_link);
    else
        pepper_list_insert(view->compositor->root_view_list.prev, &view->parent_link);

    view_geometry_dirty(view);
}

PEPPER_API pepper_object_t *
pepper_view_get_parent(pepper_object_t *v)
{
    pepper_view_t *view = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return &view->parent->base;
}

static pepper_list_t *
view_insert(pepper_view_t *view, pepper_list_t *pos, pepper_bool_t subtree)
{
    if (pos->next != &view->z_link)
    {
        pepper_list_remove(&view->z_link, NULL);
        pepper_list_insert(pos, &view->z_link);

        if (view->visibility)
            pepper_compositor_add_damage(view->compositor, &view->visible_region);
    }

    pos = &view->z_link;

    if (subtree)
    {
        pepper_list_t *l;

        PEPPER_LIST_FOR_EACH(&view->children_list, l)
            pos = view_insert((pepper_view_t *)l->item, pos, subtree);
    }

    return pos;
}

PEPPER_API pepper_bool_t
pepper_view_stack_above(pepper_object_t *v, pepper_object_t *b, pepper_bool_t subtree)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    pepper_view_t *below = (pepper_view_t *)b;

    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    CHECK_MAGIC_AND_NON_NULL(b, PEPPER_VIEW);

    view_insert(view, &below->z_link, subtree);
    return PEPPER_TRUE;
}

PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_object_t *v, pepper_object_t *a, pepper_bool_t subtree)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    pepper_view_t *above = (pepper_view_t *)a;

    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    CHECK_MAGIC_AND_NON_NULL(a, PEPPER_VIEW);

    view_insert(view, above->z_link.prev, subtree);
    return PEPPER_TRUE;
}

PEPPER_API void
pepper_view_stack_top(pepper_object_t *v, pepper_bool_t subtree)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    view_insert(view, view->compositor->view_list.prev, subtree);
}

PEPPER_API void
pepper_view_stack_bottom(pepper_object_t *v, pepper_bool_t subtree)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    view_insert(view, &view->compositor->view_list, subtree);
}

PEPPER_API pepper_object_t *
pepper_view_get_above(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return view->z_link.next->item;
}

PEPPER_API pepper_object_t *
pepper_view_get_below(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return view->z_link.prev->item;
}

PEPPER_API const pepper_list_t *
pepper_view_get_children_list(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return &view->children_list;
}

PEPPER_API void
pepper_view_resize(pepper_object_t *v, int w, int h)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    if (view->w == w && view->h == h)
        return;

    view->w = w;
    view->h = h;
    view_geometry_dirty(view);
}

PEPPER_API void
pepper_view_get_size(pepper_object_t *v, int *w, int *h)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    if (w)
        *w = view->w;

    if (h)
        *h = view->h;
}

PEPPER_API void
pepper_view_set_position(pepper_object_t *v, double x, double y)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    if (view->x == x && view->y == y)
        return;

    view->x = x;
    view->y = y;
    view_geometry_dirty(view);
}

PEPPER_API void
pepper_view_get_position(pepper_object_t *v, double *x, double *y)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    if (x)
        *x = view->x;

    if (y)
        *y = view->y;
}

PEPPER_API void
pepper_view_set_transform(pepper_object_t *v, const pepper_mat4_t *matrix)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);

    pepper_mat4_copy(&view->transform, matrix);
    view_geometry_dirty(view);
}

PEPPER_API const pepper_mat4_t *
pepper_view_get_transform(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return &view->transform;
}

PEPPER_API void
pepper_view_map(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    view_map(view);
}

PEPPER_API void
pepper_view_unmap(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    view_unmap(view);
}

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return view->mapped;
}

PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_object_t *v)
{
    pepper_view_t *view  = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return view->visibility;
}

PEPPER_API const pixman_region32_t *
pepper_view_get_visible_region(pepper_object_t *v)
{
    pepper_view_t *view = (pepper_view_t *)v;
    CHECK_MAGIC_AND_NON_NULL(v, PEPPER_VIEW);
    return &view->visible_region;
}

static void
view_update_bounding_region(pepper_view_t *view)
{
    /* TODO: */
}

static void
view_update_opaque_region(pepper_view_t *view)
{
    /* TODO: */
}

static void
damage_region_transform(pixman_region32_t *region, const pepper_mat4_t *matrix)
{
    /* TODO: */
}

void
view_update_geometry(pepper_view_t *view)
{
    if (view->parent)
        view_update_geometry(view->parent);

    if (view->geometry_dirty)
    {
        view->matrix_to_parent.m[ 0] = view->transform.m[ 0] + view->transform.m[12] * view->x;
        view->matrix_to_parent.m[ 1] = view->transform.m[ 1] + view->transform.m[13] * view->x;
        view->matrix_to_parent.m[ 2] = view->transform.m[ 2] + view->transform.m[14] * view->x;
        view->matrix_to_parent.m[ 3] = view->transform.m[ 3] + view->transform.m[15] * view->x;

        view->matrix_to_parent.m[ 4] = view->transform.m[ 4] + view->transform.m[12] * view->y;
        view->matrix_to_parent.m[ 5] = view->transform.m[ 5] + view->transform.m[13] * view->y;
        view->matrix_to_parent.m[ 6] = view->transform.m[ 6] + view->transform.m[14] * view->y;
        view->matrix_to_parent.m[ 7] = view->transform.m[ 7] + view->transform.m[15] * view->y;

        view->matrix_to_parent.m[ 8] = view->transform.m[ 8];
        view->matrix_to_parent.m[ 9] = view->transform.m[ 9];
        view->matrix_to_parent.m[10] = view->transform.m[10];
        view->matrix_to_parent.m[11] = view->transform.m[11];

        view->matrix_to_parent.m[12] = view->transform.m[12];
        view->matrix_to_parent.m[13] = view->transform.m[13];
        view->matrix_to_parent.m[14] = view->transform.m[14];
        view->matrix_to_parent.m[15] = view->transform.m[15];

        if (view->parent)
        {
            pepper_mat4_multiply(&view->matrix_to_global,
                                   &view->parent->matrix_to_global, &view->matrix_to_parent);
        }
        else
        {
            pepper_mat4_copy(&view->matrix_to_global, &view->matrix_to_parent);
        }

        view_update_bounding_region(view);
        view_update_opaque_region(view);

        view->geometry_dirty = PEPPER_FALSE;
    }
}

void
pepper_compositor_update_views(pepper_compositor_t *compositor)
{
    pepper_list_t      *l;
    pixman_region32_t   visible;
    pixman_region32_t   opaque;
    pixman_region32_t   surface_damage;
    pixman_region32_t   damage;

    pixman_region32_init(&visible);
    pixman_region32_init(&opaque);
    pixman_region32_init(&surface_damage);
    pixman_region32_init(&damage);

    /* Update views from front to back. */
    PEPPER_LIST_FOR_EACH_REVERSE(&compositor->view_list, l)
    {
        pepper_view_t *view = l->item;

        view_update_geometry(view);

        /* Calculate updated visible region. */
        pixman_region32_subtract(&visible, &view->bounding_region, &opaque);
        pixman_region32_subtract(&damage, &visible, &view->visible_region);

        /* Inflict damage for the visible region change. */
        pepper_compositor_add_damage(view->compositor, &damage);

        /* Update visible region of the view. */
        pixman_region32_copy(&view->visible_region, &visible);

        /* Inflict surface damage. */
        if (pixman_region32_not_empty(&view->surface->damage_region))
        {
            pepper_surface_flush_damage(view->surface);

            pixman_region32_copy(&surface_damage, &view->surface->damage_region);

            /* Transform surface damage into global coordinate space. */
            damage_region_transform(&surface_damage, &view->matrix_to_global);

            /* Clip surface damage with view's bounding region. */
            pixman_region32_intersect(&surface_damage, &surface_damage, &view->bounding_region);

            /* Subtract area covered by opaque views. */
            pixman_region32_subtract(&surface_damage, &surface_damage, &opaque);

            pepper_compositor_add_damage(view->compositor, &surface_damage);
        }

        /* Accumulate opaque region. */
        pixman_region32_union(&opaque, &opaque, &view->opaque_region);
    }

    pixman_region32_fini(&visible);
    pixman_region32_fini(&opaque);
    pixman_region32_fini(&surface_damage);
    pixman_region32_fini(&damage);
}
