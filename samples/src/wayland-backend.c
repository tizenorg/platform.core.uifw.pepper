#include <pepper.h>
#include <pepper-wayland.h>

/* TODO: */
#define PEPPER_ASSERT(exp)

int
main(int argc, char **argv)
{
    pepper_compositor_t    *compositor;
    pepper_wayland_t       *conn;
    pepper_output_t        *output;
    struct wl_display      *display;

    compositor = pepper_compositor_create("wayland-1");
    PEPPER_ASSERT(compositor);

    conn = pepper_wayland_connect(compositor, "wayland-0");
    PEPPER_ASSERT(conn);

    output = pepper_wayland_output_create(conn, 640, 480, "pixman");
    PEPPER_ASSERT(output);

    display = pepper_compositor_get_display(compositor);
    PEPPER_ASSERT(display);

    wl_display_run(display);

    return 0;
}
