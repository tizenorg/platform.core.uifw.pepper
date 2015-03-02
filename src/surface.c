#include "pepper_internal.h"

void *
pepper_surface_get_buffer(pepper_surface_t *surface)
{
    return surface->buffer;
}
