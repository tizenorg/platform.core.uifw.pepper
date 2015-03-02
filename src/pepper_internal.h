#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include "common.h"
#include "pepper.h"
#include <wayland-server.h>

struct pepper_compositor
{
    char		*socket_name;
    struct wl_display	*display;
};

struct pepper_output
{
    pepper_compositor_t	*compositor;

    int	x;
    int y;
    int w;
    int h;
};

struct pepper_client
{
    pepper_compositor_t *compositor;
};

struct pepper_surface
{
    void    *buffer;
};

#endif /* PEPPER_INTERNAL_H */
