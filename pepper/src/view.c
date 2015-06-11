#include "pepper-internal.h"
#include <string.h>

static void
handle_surface_destroy(struct wl_listener *listener, void *data)
{
    pepper_view_t *view = pepper_container_of(listener, pepper_view_t, surface_destroy_listener);
    PEPPER_ASSERT(view->surface != NULL);
    pepper_view_destroy(view);
}

static void
damage_region_transform(pixman_region32_t *region, pepper_matrix_t *matrix)
{
    /* TODO: */
}

static void
view_damage_below_consume(pepper_view_t *view)
{
    pepper_compositor_add_damage(view->compositor, &view->visible_region);
    pixman_region32_init(&view->visible_region);
}

static void
view_geometry_dirty(pepper_view_t *view)
{
    pepper_view_t *child;

    if (view->geometry_dirty)
        return;

    view->geometry_dirty = PEPPER_TRUE;

    view_damage_below_consume(view);
    view->need_damage = PEPPER_TRUE;

    wl_list_for_each(child, &view->child_list, parent_link)
        view_geometry_dirty(child);
}

PEPPER_API pepper_view_t *
pepper_compositor_add_view(pepper_compositor_t *compositor,
                           pepper_view_t *parent, pepper_view_t *pos, pepper_surface_t *surface)
{
    pepper_view_t *view;

    if (!compositor)
    {
        PEPPER_ERROR("Compositor must be given.\n");
        return NULL;
    }

    view = pepper_calloc(1, sizeof(pepper_view_t));
    if (!view)
    {
        PEPPER_ERROR("Failed to allocate memory.\n");
        return NULL;
    }

    view->compositor = compositor;
    wl_signal_init(&view->destroy_signal);

    view->parent = parent;

    wl_list_init(&view->child_list);
    pepper_matrix_load_identity(&view->transform);
    view->alpha = 1.0f;

    view->surface = surface;

    if (surface)
    {
        wl_list_insert(surface->view_list.next, &view->surface_link);
        view->surface_destroy_listener.notify = handle_surface_destroy;
        pepper_surface_add_destroy_listener(surface, &view->surface_destroy_listener);
    }
    else
    {
        wl_list_init(&view->surface_link);
    }

    pixman_region32_init(&view->clip_region);
    pixman_region32_init(&view->visible_region);

    if (pos)
        wl_list_insert(pos->parent_link.next, &view->parent_link);
    else if (parent)
        wl_list_insert(parent->child_list.next, &view->parent_link);
    else
        wl_list_insert(compositor->root_view_list.next, &view->parent_link);

    if (parent)
        view->container_list = &parent->child_list;
    else
        view->container_list = &compositor->root_view_list;

    view_geometry_dirty(view);
    return view;
}

PEPPER_API pepper_view_t *
pepper_compositor_get_top_root_view(pepper_compositor_t *compositor)
{
    if (wl_list_empty(&compositor->root_view_list))
        return NULL;

    return pepper_container_of(compositor->root_view_list.prev, pepper_view_t, parent_link);
}

PEPPER_API pepper_view_t *
pepper_compositor_get_bottom_root_view(pepper_compositor_t *compositor)
{
    if (wl_list_empty(&compositor->root_view_list))
        return NULL;

    return pepper_container_of(compositor->root_view_list.next, pepper_view_t, parent_link);
}

static void
pepper_view_unmap(pepper_view_t *view)
{
    pepper_view_t *child;

    if (!view->visibility)
        return;

    wl_list_for_each(child, &view->child_list, parent_link)
        pepper_view_unmap(child);

    view->visibility = PEPPER_FALSE;
    view_damage_below_consume(view);
}

PEPPER_API void
pepper_view_destroy(pepper_view_t *view)
{
    pepper_view_t *child, *next;

    pepper_view_unmap(view);

    /* Destroy all child views. */
    wl_list_for_each_safe(child, next, &view->child_list, parent_link)
        pepper_view_destroy(child);

    wl_list_remove(&view->parent_link);

    if (view->surface)
        wl_list_remove(&view->surface_link);

    pixman_region32_fini(&view->clip_region);

    /* Emit that the view is going to be destroyed. */
    wl_signal_emit(&view->destroy_signal, view);

    pepper_free(view);
}

PEPPER_API void
pepper_view_add_destroy_listener(pepper_view_t *view, struct wl_listener *listener)
{
    wl_signal_add(&view->destroy_signal, listener);
}

PEPPER_API pepper_compositor_t *
pepper_view_get_compositor(pepper_view_t *view)
{
    return view->compositor;
}

PEPPER_API pepper_view_t *
pepper_view_get_parent(pepper_view_t *view)
{
    return view->parent;
}

PEPPER_API pepper_surface_t *
pepper_view_get_surface(pepper_view_t *view)
{
    return view->surface;
}

PEPPER_API pepper_bool_t
pepper_view_stack_above(pepper_view_t *view, pepper_view_t *below)
{
    if (view == below)
        return PEPPER_TRUE;

    if (view->parent != below->parent)
        return PEPPER_FALSE;

    wl_list_remove(&view->parent_link);
    wl_list_insert(below->parent_link.next, &view->parent_link);

    view_damage_below_consume(view);
    view->need_damage = PEPPER_TRUE;

    return PEPPER_TRUE;
}

PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_view_t *view, pepper_view_t *above)
{
    if (view == above)
        return PEPPER_TRUE;

    if (view->parent != above->parent)
        return PEPPER_FALSE;

    wl_list_remove(&view->parent_link);
    wl_list_insert(above->parent_link.prev, &view->parent_link);

    view_damage_below_consume(view);
    view->need_damage = PEPPER_TRUE;

    return PEPPER_TRUE;
}

PEPPER_API void
pepper_view_stack_top(pepper_view_t *view)
{
    if (view->container_list->prev == &view->parent_link)
        return;

    wl_list_remove(&view->parent_link);
    wl_list_insert(view->container_list->prev, &view->parent_link);

    view_damage_below_consume(view);
    view->need_damage = PEPPER_TRUE;
}

PEPPER_API void
pepper_view_stack_bottom(pepper_view_t *view)
{
    if (view->container_list->next == &view->parent_link)
        return;

    wl_list_remove(&view->parent_link);
    wl_list_insert(view->container_list, &view->parent_link);

    view_damage_below_consume(view);
    view->need_damage = PEPPER_TRUE;
}

PEPPER_API pepper_view_t *
pepper_view_get_above(pepper_view_t *view)
{
    if (view->parent_link.next == view->container_list)
        return NULL;

    return pepper_container_of(view->parent_link.next, pepper_view_t, parent_link);
}

PEPPER_API pepper_view_t *
pepper_view_get_below(pepper_view_t *view)
{
    if (view->parent_link.next == view->container_list)
        return NULL;

    return pepper_container_of(view->parent_link.prev, pepper_view_t, parent_link);
}


PEPPER_API pepper_view_t *
pepper_view_get_top_child(pepper_view_t *view)
{
    if (wl_list_empty(&view->child_list))
        return NULL;

    return pepper_container_of(view->child_list.prev, pepper_view_t, parent_link);
}

PEPPER_API pepper_view_t *
pepper_view_get_bottom_child(pepper_view_t *view)
{
    if (wl_list_empty(&view->child_list))
        return NULL;

    return pepper_container_of(view->child_list.next, pepper_view_t, parent_link);
}

PEPPER_API void
pepper_view_resize(pepper_view_t *view, float w, float h)
{
    if (w < 0.0f)
        w = 0.0f;

   if (h < 0.0f)
       h = 0.0f;

    view->w = w;
    view->h = h;

    view_geometry_dirty(view);
}

PEPPER_API void
pepper_view_get_size(pepper_view_t *view, float *w, float *h)
{
    if (w)
        *w = view->w;

    if (h)
        *h = view->h;
}

PEPPER_API void
pepper_view_set_position(pepper_view_t *view, float x, float y)
{
    view->x = x;
    view->y = y;

    view_geometry_dirty(view);
}

PEPPER_API void
pepper_view_get_position(pepper_view_t *view, float *x, float *y)
{
    if (x)
        *x = view->x;

    if (y)
        *y = view->y;
}

PEPPER_API void
pepper_view_set_transform(pepper_view_t *view, const pepper_matrix_t *matrix)
{
    memcpy(&view->transform, matrix, sizeof(pepper_matrix_t));
    view_geometry_dirty(view);
}

PEPPER_API const pepper_matrix_t *
pepper_view_get_transform(pepper_view_t *view)
{
    return &view->transform;
}

PEPPER_API void
pepper_view_set_visibility(pepper_view_t *view, pepper_bool_t visibility)
{
    if (view->visibility == visibility)
        return;

    view->visibility = visibility;
    view_damage_below_consume(view);
}

PEPPER_API pepper_bool_t
pepper_view_get_visibility(pepper_view_t *view)
{
    return view->visibility;
}

PEPPER_API void
pepper_view_set_alpha(pepper_view_t *view, float alpha)
{
    if (alpha < 0.0f)
        alpha = 0.0f;

    if (alpha > 1.0f)
        alpha = 1.0f;

    if (view->alpha == alpha)
        return;

    view->alpha = alpha;
    view_damage_below_consume(view);
}

PEPPER_API float
pepper_view_get_alpha(pepper_view_t *view)
{
    return view->alpha;
}

PEPPER_API void
pepper_view_set_viewport(pepper_view_t *view, int x, int y, int w, int h)
{
    if (view->viewport.x == x && view->viewport.y == y &&
        view->viewport.w == w && view->viewport.h == h)
        return;

    view->viewport.x = x;
    view->viewport.y = y;
    view->viewport.w = w;
    view->viewport.h = h;

    view_damage_below_consume(view);
}

PEPPER_API void
pepper_view_get_viewport(pepper_view_t *view, int *x, int *y, int *w, int *h)
{
    if (x)
        *x = view->viewport.x;

    if (y)
        *y = view->viewport.y;

    if (w)
        *w = view->viewport.w;

    if (h)
        *h = view->viewport.h;
}

PEPPER_API void
pepper_view_set_clip_to_parent(pepper_view_t *view, pepper_bool_t clip)
{
    if (view->clip_to_parent == clip)
        return;

    view->clip_to_parent = clip;
    view_damage_below_consume(view);
}

PEPPER_API pepper_bool_t
pepper_view_get_clip_to_parent(pepper_view_t *view)
{
    return view->clip_to_parent;
}

PEPPER_API pepper_bool_t
pepper_view_set_clip_region(pepper_view_t *view, const pixman_region32_t *region)
{
    if (!pixman_region32_copy(&view->clip_region, (pixman_region32_t *)region))
        return PEPPER_FALSE;

    view_damage_below_consume(view);
    return PEPPER_TRUE;
}

PEPPER_API const pixman_region32_t *
pepper_view_get_clip_region(pepper_view_t *view)
{
    return &view->clip_region;
}

static void
view_update_bounding_region(pepper_view_t *view)
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
            pepper_matrix_multiply(&view->matrix_to_global,
                                   &view->parent->matrix_to_global, &view->matrix_to_parent);
        }
        else
        {
            pepper_matrix_copy(&view->matrix_to_global, &view->matrix_to_parent);
        }

        view_update_bounding_region(view);
        view->geometry_dirty = PEPPER_FALSE;
    }
}

static void
view_list_add(pepper_view_t *view)
{
    pepper_view_t *child;

    wl_list_insert(&view->compositor->view_list, &view->view_list_link);

    wl_list_for_each(child, &view->child_list, parent_link)
        view_list_add(child);
}

void
pepper_compositor_update_view_list(pepper_compositor_t *compositor)
{
    pepper_view_t      *view;
    pixman_region32_t   visible;
    pixman_region32_t   opaque;
    pixman_region32_t   surface_damage;

    pixman_region32_init(&visible);
    pixman_region32_init(&opaque);
    pixman_region32_init(&surface_damage);

    /* Make compositor's view list empty. */
    wl_list_init(&compositor->view_list);

    /* Build z-ordered view list by traversing the view tree in depth-first order. */
    wl_list_for_each(view, &compositor->root_view_list, parent_link)
        view_list_add(view);

    /* Update views from front to back. */
    wl_list_for_each_reverse(view, &compositor->view_list, view_list_link)
    {
        view_update_geometry(view);

        /* Update visible region. */
        pixman_region32_subtract(&view->visible_region, &view->bounding_region, &opaque);

        /* Inflict damage caused by geometry and z-order change. */
        if (view->need_damage)
        {
            pepper_compositor_add_damage(view->compositor, &view->visible_region);
            view->need_damage = PEPPER_FALSE;
        }

        /* Inflict surface damage. */
        if (pixman_region32_not_empty(&view->surface->damage_region))
        {
            pepper_surface_flush_damage(view->surface);

            /* Intersect surface damage region with viewport rectangle. */
            pixman_region32_intersect_rect(&surface_damage, &view->surface->damage_region,
                                           view->viewport.x, view->viewport.y,
                                           view->viewport.w, view->viewport.h);

            /* Translate surface damage to compensate viewport offset. */
            pixman_region32_translate(&surface_damage, -view->viewport.x, -view->viewport.y);

            /* Transform surface damage into global coordinate space. */
            damage_region_transform(&surface_damage, &view->matrix_to_global);

            /* Subtract area covered by opaque views. */
            pixman_region32_subtract(&surface_damage, &surface_damage, &opaque);

            pepper_compositor_add_damage(view->compositor, &surface_damage);
        }

        /* Accumulate opaque region. */
        pixman_region32_union(&opaque, &opaque, &view->opaque_region);
    }
}
