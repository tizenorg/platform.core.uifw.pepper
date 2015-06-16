#include <pepper.h>
#include <pepper-x11.h>

/* TODO: */
#define PEPPER_ASSERT(exp)

int
main(int argc, char **argv)
{
    pepper_object_t         *compositor;
    pepper_object_t         *output;
    pepper_output_mode_t     mode;
    pepper_x11_connection_t *conn;

    struct wl_display       *display;

    compositor = pepper_compositor_create("wayland-1");
    PEPPER_ASSERT(compositor);

    conn = pepper_x11_connect(compositor, NULL);
    PEPPER_ASSERT(conn);

    output = pepper_x11_output_create(conn, 640, 480, "pixman");
    PEPPER_ASSERT(output);

#if 0
    mode.w = 320;
    mode.h = 240;
    mode.refresh = 60000;
    pepper_output_set_mode(output, &mode);
#endif

    display = pepper_compositor_get_display(compositor);
    PEPPER_ASSERT(display);

    wl_display_run(display);

    return 0;
}
