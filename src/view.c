#include "pepper-internal.h"
#include <string.h>

void
view_geometry_dirty(pepper_view_t *view)
{
    pepper_view_t *child;

    if (view->geometry.dirty)
        return;

    view->geometry.dirty = PEPPER_TRUE;

    wl_list_for_each(child, &view->childs, parent_link)
        view_geometry_dirty(child);
}

static void
handle_parent_destroy(struct wl_listener *listener, void *data)
{
    pepper_view_t *view = wl_container_of(listener, view, parent_destroy_listener);

    PEPPER_ASSERT(view->parent == data);
    pepper_view_set_parent(view, NULL);
}

PEPPER_API pepper_view_t *
pepper_view_create(pepper_compositor_t *compositor, pepper_surface_t *surface)
{
    pepper_view_t *view;

    if (!compositor)
    {
        PEPPER_ERROR("Compositor must be given.\n");
        return NULL;
    }

    if (surface && surface->compositor != compositor)
    {
        PEPPER_ERROR("Unable to create a view for a surface from different compositor.\n");
        return NULL;
    }

    view = pepper_calloc(1, sizeof(pepper_view_t));
    if (!view)
        return NULL;

    view->compositor = compositor;
    wl_signal_init(&view->destroy_signal);

    view->surface = surface;
    view->alpha = 1.0f;

    view->parent_destroy_listener.notify = handle_parent_destroy;
    wl_list_init(&view->childs);

    /* Initialize geometry properties. */
    view->geometry.dirty = PEPPER_TRUE;
    view->geometry.x = 0.0f;
    view->geometry.y = 0.0f;
    pepper_matrix_load_identity(&view->geometry.transform);

    return view;
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
pepper_view_destroy(pepper_view_t *view)
{
    wl_signal_emit(&view->destroy_signal, view);
    PEPPER_ASSERT(wl_list_empty(&view->childs));

    if (view->parent)
    {
        wl_list_remove(&view->parent_link);
        wl_list_remove(&view->parent_destroy_listener.link);
    }

    pepper_free(view);
}

PEPPER_API void
pepper_view_add_destroy_listener(pepper_view_t *view, struct wl_listener *listener)
{
    wl_signal_add(&view->destroy_signal, listener);
}

PEPPER_API void
pepper_view_set_position(pepper_view_t *view, float x, float y)
{
    view->geometry.x = x;
    view->geometry.y = y;
}

PEPPER_API void
pepper_view_get_position(pepper_view_t *view, float *x, float *y)
{
    if (x)
        *x = view->geometry.x;

    if (y)
        *y = view->geometry.y;
}

PEPPER_API void
pepper_view_set_transform(pepper_view_t *view, const pepper_matrix_t *matrix)
{
    memcpy(&view->geometry.transform, matrix, sizeof(pepper_matrix_t));
    view_geometry_dirty(view);
}

PEPPER_API const pepper_matrix_t *
pepper_view_get_transform(pepper_view_t *view)
{
    return &view->geometry.transform;
}

PEPPER_API void
pepper_view_set_parent(pepper_view_t *view, pepper_view_t *parent)
{
    if (view->parent == parent)
        return;

    if (view->parent)
    {
        wl_list_remove(&view->parent_link);
        wl_list_remove(&view->parent_destroy_listener.link);
    }

    view->parent = parent;

    if (parent)
    {
        wl_list_insert(&parent->childs, &view->parent_link);
        pepper_view_add_destroy_listener(parent, &view->parent_destroy_listener);
    }

    view_geometry_dirty(view);
}

PEPPER_API pepper_view_t *
pepper_view_get_parent(pepper_view_t *view)
{
    return view->parent;
}

PEPPER_API void
pepper_view_map(pepper_view_t *view)
{
    if (view->mapped)
        return;

    view->mapped = PEPPER_TRUE;
}

PEPPER_API void
pepper_view_unmap(pepper_view_t *view)
{
    if (!view->mapped)
        return;

    view->mapped = PEPPER_FALSE;
}

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_view_t *view)
{
    return view->mapped;
}

PEPPER_API void
pepper_view_set_alpha(pepper_view_t *view, float alpha)
{
    if (view->alpha == alpha)
        return;

    view->alpha = alpha;
}

PEPPER_API float
pepper_view_get_alpha(pepper_view_t *view)
{
    return view->alpha;
}

PEPPER_API pepper_view_t *
pepper_view_get_above(pepper_view_t *view)
{
    pepper_view_t *above;

    if (!view->layer)
        return NULL;

    if (view->layer_link.next == &view->layer->views)
        return NULL;

    above = wl_container_of(view->layer_link.next, view, layer_link);
    return above;
}

PEPPER_API pepper_view_t *
pepper_view_get_below(pepper_view_t *view)
{
    pepper_view_t *below;

    if (!view->layer)
        return NULL;

    if (view->layer_link.prev == &view->layer->views)
        return NULL;

    below = wl_container_of(view->layer_link.prev, view, layer_link);
    return below;
}
