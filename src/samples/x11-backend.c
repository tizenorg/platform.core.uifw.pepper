#include <pepper.h>
#include <pepper-x11.h>
#include <pepper-desktop-shell.h>

int
main(int argc, char **argv)
{
    pepper_compositor_t     *compositor;
    pepper_output_t         *output;
    pepper_output_mode_t     mode;
    pepper_x11_connection_t *conn;

    struct wl_display       *display;

    compositor = pepper_compositor_create("wayland-1");
    PEPPER_ASSERT(compositor);

    conn = pepper_x11_connect(compositor, NULL);
    PEPPER_ASSERT(conn);

    output = pepper_x11_output_create(conn, 640, 480, "pixman");
    PEPPER_ASSERT(output);

    mode.w = 1024;
    mode.h = 768;
    mode.refresh = 60000;
    pepper_output_set_mode(output, &mode);

    if (!pepper_desktop_shell_init(compositor))
        PEPPER_ASSERT(0);

    display = pepper_compositor_get_display(compositor);
    PEPPER_ASSERT(display);

    wl_display_run(display);

    return 0;
}
