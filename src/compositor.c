#include "pepper_internal.h"

pepper_compositor_t *
pepper_compositor_create(const char *socket_name,
			 const char *backend_name,
			 const char *input_name,
			 const char *shell_name,
			 const char *renderer_name)
{
    pepper_compositor_t	*compositor = NULL;

    compositor = (pepper_compositor_t *)calloc(1, sizeof (pepper_compositor_t));

    if (!compositor)
    {
	PEPPER_ERROR("Memory allocation failed!!!\n");
	goto error;
    }

    compositor->display = wl_display_create();

    if (!compositor->display)
    {
	PEPPER_ERROR("Failed to create wayland display object.\n");
	goto error;
    }

    if (wl_display_add_socket(compositor->display, socket_name) != 0)
    {
	PEPPER_ERROR("Failed to add socket display = %p socket_name = %s\n",
		     compositor->display, socket_name);
	goto error;
    }

    /* TODO: Load modules. */

    return compositor;

error:
    if (compositor)
	pepper_compositor_destroy(compositor);

    return NULL;
}

void
pepper_compositor_destroy(pepper_compositor_t *compositor)
{
    if (compositor->display)
	wl_display_destroy(compositor->display);

    free(compositor);
}

pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t  *compositor,
			     pepper_output_info_t *info)
{
    pepper_output_t *output = NULL;

    output = (pepper_output_t *)calloc(1, sizeof (pepper_output_t));

    if (!output)
    {
	PEPPER_ERROR("Memory allocation failed!!!\n");
	goto error;
    }

    /* TODO: Backend-size output initialization. */

    output->x = info->x;
    output->y = info->y;
    output->w = info->w;
    output->h = info->h;

    /* TODO: Add to compositor's output list. */

    return output;

error:
    if (output)
	pepper_output_destroy(output);

    return NULL;
}

pepper_client_t *
pepper_compositor_get_client(pepper_compositor_t *compositor, int index)
{
    /* TODO: */
    return NULL;
}

int
pepper_compositor_get_output_count(pepper_compositor_t *compositor)
{
    /* TODO: */
    return 0;
}

pepper_output_t *
pepper_compositor_get_output(pepper_compositor_t *compositor, int index)
{
    /* TODO: */
    return NULL;
}

void
pepper_compositor_frame(pepper_compositor_t *compositor)
{
    /* TODO: */
}
