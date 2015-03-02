#include "pepper_internal.h"

pepper_bool_t
pepper_output_move(pepper_output_t *output, int x, int y, int w, int h)
{
    /* TODO: */
    return PEPPER_FALSE;
}

void
pepper_output_get_geometry(pepper_output_t *output, int *x, int *y, int *w, int *h)
{
    if (x)
	*x = output->x;

    if (y)
	*y = output->y;

    if (w)
	*w = output->w;

    if (h)
	*h = output->h;
}

pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output)
{
    return output->compositor;
}

pepper_bool_t
pepper_output_destroy(pepper_output_t *output)
{
    /* TODO: */
    return PEPPER_FALSE;
}
