#include "pepper-render-internal.h"

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer)
{
    renderer->destroy(renderer);
}

PEPPER_API void
pepper_renderer_repaint_output(pepper_renderer_t *renderer, pepper_object_t *output)
{
    renderer->repaint_output(renderer, output);
}
