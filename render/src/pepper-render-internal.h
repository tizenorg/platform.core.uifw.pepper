#ifndef PEPPER_RENDER_INTERNAL_H
#define PEPPER_RENDER_INTERNAL_H

#include "pepper-render.h"

/* TODO: Error logging. */
#define PEPPER_ASSERT(exp)
#define PEPPER_ERROR(...)

struct pepper_renderer
{
    pepper_compositor_t    *compositor;

    void            (*destroy)(pepper_renderer_t *renderer);

    pepper_bool_t   (*attach_surface)(pepper_renderer_t *renderer,
                                      pepper_surface_t *surface, int *w, int *h);

    void            (*flush_surface_damage)(pepper_renderer_t *renderer,
                                            pepper_surface_t *surface);

    pepper_bool_t   (*read_pixels)(pepper_renderer_t *renderer,
                                   int x, int y, int w, int h,
                                   void *pixels, pepper_format_t format);

    void            (*repaint_output)(pepper_renderer_t *renderer, pepper_output_t *output);
};

#endif /* PEPPER_RENDER_INTERNAL_H */
