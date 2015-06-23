#include "pepper-render-internal.h"

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer)
{
    renderer->destroy(renderer);
}

PEPPER_API void
pepper_render_target_destroy(pepper_render_target_t *target)
{
    target->destroy(target);
}

PEPPER_API pepper_bool_t
pepper_renderer_set_target(pepper_renderer_t *renderer, pepper_render_target_t *target)
{
    if (target->renderer != NULL && target->renderer != renderer)
        return PEPPER_FALSE;

    renderer->target = target;
    return PEPPER_TRUE;
}

PEPPER_API pepper_render_target_t *
pepper_renderer_get_target(pepper_renderer_t *renderer)
{
    return renderer->target;
}

PEPPER_API pepper_bool_t
pepper_renderer_attach_surface(pepper_renderer_t *renderer,
                               pepper_object_t *surface, int *w, int *h)
{
    return renderer->attach_surface(renderer, surface, w, h);
}

PEPPER_API pepper_bool_t
pepper_renderer_flush_surface_damage(pepper_renderer_t *renderer, pepper_object_t *surface)
{
    return renderer->flush_surface_damage(renderer, surface);
}

PEPPER_API void
pepper_renderer_repaint_output(pepper_renderer_t *renderer, pepper_object_t *output,
                               const pepper_list_t *view_list, const pixman_region32_t *damage)
{
    renderer->repaint_output(renderer, output, view_list, damage);
}

PEPPER_API pepper_bool_t
pepper_renderer_read_pixels(pepper_renderer_t *renderer, int x, int y, int w, int h,
                            void *pixels, pepper_format_t format)
{
    return renderer->read_pixels(renderer, x, y, w, h, pixels, format);
}
