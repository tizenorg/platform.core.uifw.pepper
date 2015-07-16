#include "pepper-internal.h"

void
pepper_plane_update(pepper_plane_t *plane, const pepper_list_t *view_list)
{
    pepper_list_t  *l;
    double          output_x = plane->output->geometry.x;
    double          output_y = plane->output->geometry.y;

    pepper_list_init(&plane->entry_list);

    PEPPER_LIST_FOR_EACH(view_list, l)
    {
        pepper_view_t          *view  = l->item;
        pepper_plane_entry_t   *entry = &view->plane_entries[plane->output->id];

        if (entry->plane == plane)
        {
            pepper_list_insert(&plane->entry_list, &entry->link);

            /* Calculate view transform on output local coordinate space. */
            pepper_mat4_init_translate(&entry->base.transform, -output_x, -output_y, 0.0);
            pepper_mat4_multiply(&entry->base.transform,
                                 &entry->base.transform, &view->global_transform);
        }
    }
}

void
pepper_plane_accumulate_damage(pepper_plane_t *plane, pixman_region32_t *clip)
{
    pepper_list_t  *l;
    int             x = plane->output->geometry.x;
    int             y = plane->output->geometry.y;
    int             w = plane->output->geometry.w;
    int             h = plane->output->geometry.h;

    pixman_region32_init(clip);

    PEPPER_LIST_FOR_EACH_REVERSE(&plane->entry_list, l)
    {
        pepper_plane_entry_t   *entry = l->item;
        pepper_view_t          *view = (pepper_view_t *)entry->base.view;

        pixman_region32_subtract(&entry->base.visible_region, &view->bounding_region, clip);
        pixman_region32_translate(&entry->base.visible_region, -x, -y);
        pixman_region32_intersect_rect(&entry->base.visible_region,
                                       &entry->base.visible_region, 0, 0, w, h);

        pixman_region32_union(clip, clip, &view->opaque_region);

        if (entry->need_damage)
        {
            pepper_view_damage_below(view);
            entry->need_damage = PEPPER_FALSE;
        }
    }

    pixman_region32_translate(clip, -x, -y);
    pixman_region32_intersect_rect(clip, clip, 0, 0, w, h);
}

PEPPER_API pepper_plane_t *
pepper_output_add_plane(pepper_output_t *output, pepper_plane_t *above)
{
    pepper_plane_t *plane;

    if (above && above->output != output)
        return NULL;

    plane = (pepper_plane_t *)pepper_object_alloc(sizeof(pepper_plane_t));
    if (!plane)
        return NULL;

    plane->output = output;
    plane->link.item = plane;

    if (above)
        pepper_list_insert(above->link.prev, &plane->link);
    else
        pepper_list_insert(output->plane_list.prev, &plane->link);

    pepper_list_init(&plane->entry_list);
    pixman_region32_init(&plane->damage_region);
    pixman_region32_init(&plane->clip_region);

    return plane;
}

PEPPER_API void
pepper_plane_destroy(pepper_plane_t *plane)
{
    pepper_list_t  *l;

    pepper_object_fini(&plane->base);

    PEPPER_LIST_FOR_EACH(&plane->entry_list, l)
    {
        pepper_plane_entry_t *entry = l->item;
        pepper_view_assign_plane(entry->base.view, plane->output, NULL);
    }

    pepper_list_remove(&plane->link, NULL);
    pixman_region32_fini(&plane->damage_region);
    pixman_region32_fini(&plane->clip_region);

    pepper_free(plane);
}

void
pepper_plane_add_damage_region(pepper_plane_t *plane, pixman_region32_t *damage)
{
    if (!damage)
    {
        pixman_region32_union_rect(&plane->damage_region, &plane->damage_region,
                                   0, 0, plane->output->geometry.w, plane->output->geometry.h);
        pepper_output_schedule_repaint(plane->output);
    }
    else if (pixman_region32_not_empty(damage))
    {
        pixman_region32_union(&plane->damage_region, &plane->damage_region, damage);
        pepper_output_schedule_repaint(plane->output);
    }
}

PEPPER_API pixman_region32_t *
pepper_plane_get_damage_region(pepper_plane_t *plane)
{
    return &plane->damage_region;
}

PEPPER_API pixman_region32_t *
pepper_plane_get_clip_region(pepper_plane_t *plane)
{
    return &plane->clip_region;
}

PEPPER_API const pepper_list_t *
pepper_plane_get_render_list(pepper_plane_t *plane)
{
    return &plane->entry_list;
}

PEPPER_API void
pepper_plane_subtract_damage_region(pepper_plane_t *plane, pixman_region32_t *damage)
{
    pixman_region32_subtract(&plane->damage_region, &plane->damage_region, damage);
}

PEPPER_API void
pepper_plane_clear_damage_region(pepper_plane_t *plane)
{
    pixman_region32_clear(&plane->damage_region);
}
