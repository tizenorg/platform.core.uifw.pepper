#include "pepper-internal.h"

PEPPER_API pepper_layer_t *
pepper_layer_create(pepper_compositor_t *compositor)
{
    pepper_layer_t *layer;
    PEPPER_ASSERT(compositor != NULL);

    layer = pepper_calloc(1, sizeof(pepper_layer_t));
    if (!layer)
        return NULL;

    layer->compositor = compositor;
    wl_list_init(&layer->views);

    return layer;
}

PEPPER_API pepper_compositor_t *
pepper_layer_get_compositor(pepper_layer_t *layer)
{
    return layer->compositor;
}

PEPPER_API void
pepper_compositor_stack_layer(pepper_compositor_t *compositor, pepper_layer_t *layer,
                              pepper_layer_t *below)
{
    PEPPER_ASSERT(compositor != NULL);
    PEPPER_ASSERT(!below || (layer->compositor == below->compositor));

    if ((!below && (layer->link.prev == &compositor->layers)) ||
        ( below && (layer->link.prev == &below->link)))
        return;

    if (!wl_list_empty(&layer->link))
    {
        wl_list_remove(&layer->link);

        /* TODO: Handle layer removal. */
    }

    if (below)
        wl_list_insert(&below->link, &layer->link);
    else
        wl_list_insert(&compositor->layers, &layer->link);

    /* TODO: Handle layer insert. */
}

PEPPER_API pepper_layer_t *
pepper_compositor_get_top_layer(pepper_compositor_t *compositor)
{
    pepper_layer_t *layer;

    if (wl_list_empty(&compositor->layers))
        return NULL;

    layer = wl_container_of(compositor->layers.prev, layer, link);
    return layer;
}

PEPPER_API pepper_layer_t *
pepper_compositor_get_bottom_layer(pepper_compositor_t *compositor)
{
    pepper_layer_t *layer;

    if (wl_list_empty(&compositor->layers))
        return NULL;

    layer = wl_container_of(compositor->layers.next, layer, link);
    return layer;
}

PEPPER_API pepper_layer_t *
pepper_layer_get_above(pepper_layer_t *layer)
{
    pepper_layer_t *above;

    if (wl_list_empty(&layer->link))
        return NULL;

    if (layer->link.next == &layer->compositor->layers)
        return NULL;

    above = wl_container_of(layer->link.next, layer, link);
    return above;
}

PEPPER_API pepper_layer_t *
pepper_layer_get_below(pepper_layer_t *layer)
{
    pepper_layer_t *below;

    if (wl_list_empty(&layer->link))
        return NULL;

    if (layer->link.prev == &layer->compositor->layers)
        return NULL;

    below = wl_container_of(layer->link.prev, layer, link);
    return below;
}

PEPPER_API void
pepper_layer_stack_view(pepper_layer_t *layer, pepper_view_t *view, pepper_view_t *below)
{
    PEPPER_ASSERT(layer != NULL);
    PEPPER_ASSERT(!below || (below->compositor == view->compositor));

    if ((!below && (view->layer_link.prev == &layer->views)) ||
        ( below && (view->layer_link.prev == &below->layer_link)))
        return;

    if (view->layer)
    {
        wl_list_remove(&view->layer_link);

        /* TODO: Handle view removal. */
    }

    if (below)
        wl_list_insert(&below->layer_link, &view->layer_link);
    else
        wl_list_insert(&layer->views, &view->layer_link);

    /* TODO: Handle view insert. */
}

PEPPER_API pepper_view_t *
pepper_layer_get_top_view(pepper_layer_t *layer)
{
    pepper_view_t *view;

    if (wl_list_empty(&layer->views))
        return NULL;

    view = wl_container_of(layer->views.prev, view, layer_link);
    return view;
}

PEPPER_API pepper_view_t *
pepper_layer_get_bottom_view(pepper_layer_t *layer)
{
    pepper_view_t *view;

    if (wl_list_empty(&layer->views))
        return NULL;

    view = wl_container_of(layer->views.next, view, layer_link);
    return view;
}
