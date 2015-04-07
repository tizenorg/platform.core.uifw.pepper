#include "pepper.h"
#include <string.h>

PEPPER_API void
pepper_renderer_init(pepper_renderer_t *renderer)
{
    memset(renderer, 0x00, sizeof(pepper_renderer_t));
}

static void
pepper_renderer_fini(pepper_renderer_t *renderer)
{
    memset(renderer, 0x00, sizeof(pepper_renderer_t));
}

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer)
{
    pepper_renderer_fini(renderer);
    renderer->destroy(renderer);
}
