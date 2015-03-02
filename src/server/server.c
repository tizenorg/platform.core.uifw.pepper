#include "../common.h"
#include <pepper.h>

int
main(int argc, char **argv)
{
    pepper_compositor_t	*compositor;

    compositor = pepper_compositor_create("wayland-0", NULL, NULL, NULL, NULL);

    while (1)
    {
	pepper_compositor_frame(compositor);
    }

    pepper_compositor_destroy(compositor);

    return 0;
}
