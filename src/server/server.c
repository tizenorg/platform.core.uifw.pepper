#include "../common.h"
#include <pepper.h>

int
main(int argc, char **argv)
{
    struct wl_display   *display;
    pepper_compositor_t *compositor;

    compositor = pepper_compositor_create("wayland-0");
    display = pepper_compositor_get_display(compositor);

    /* Enter main loop. */
    wl_display_run(display);

    pepper_compositor_destroy(compositor);

    return 0;
}
