#include <pepper.h>
#include <pepper-x11.h>
#include <pepper-desktop-shell.h>
#include <signal.h>

static int
handle_sigint(int signal_number, void *data)
{
    struct wl_display *display = (struct wl_display *)data;
    wl_display_terminate(display);

    return 0;
}

int
main(int argc, char **argv)
{
    pepper_compositor_t     *compositor;
    pepper_output_t         *output;
    pepper_output_mode_t     mode;
    pepper_x11_connection_t *conn;
    struct wl_event_loop    *loop = NULL;
    struct wl_event_source  *sigint = NULL;
    struct wl_display       *display;
    const char              *socket = NULL;
    const char              *renderer;

    if (argc > 1)
        socket = argv[1];

    if (argc > 2)
        renderer = argv[2];

    compositor = pepper_compositor_create(socket);
    PEPPER_ASSERT(compositor);

    conn = pepper_x11_connect(compositor, NULL);
    PEPPER_ASSERT(conn);

    output = pepper_x11_output_create(conn, 1024, 768, renderer);
    PEPPER_ASSERT(output);

    if (!pepper_x11_input_create(conn))
        PEPPER_ASSERT(0);

    if (!pepper_desktop_shell_init(compositor))
        PEPPER_ASSERT(0);

    display = pepper_compositor_get_display(compositor);
    PEPPER_ASSERT(display);

    loop = wl_display_get_event_loop(display);
    sigint = wl_event_loop_add_signal(loop, SIGINT, handle_sigint, display);

    wl_display_run(display);

    wl_event_source_remove(sigint);
    pepper_x11_destroy(conn);
    pepper_compositor_destroy(compositor);

    return 0;
}
