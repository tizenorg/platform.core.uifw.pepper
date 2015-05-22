#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <pepper.h>
#include <pepper-drm.h>
#include <pepper-desktop-shell.h>
#include <common.h>

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
    pepper_compositor_t    *compositor;
    pepper_drm_t           *drm;
    struct wl_display      *display;
    struct wl_event_loop   *loop;
    struct wl_event_source *sigint;

    {   /* for gdb attach */
        char cc;
        scanf("%c", &cc);
    }

    compositor = pepper_compositor_create("wayland-0");
    PEPPER_ASSERT(compositor);

    drm = pepper_drm_create(compositor, "");
    PEPPER_ASSERT(drm);

    if (!pepper_desktop_shell_init(compositor))
        PEPPER_ASSERT(0);

    display = pepper_compositor_get_display(compositor);
    PEPPER_ASSERT(display);

    loop = wl_display_get_event_loop(display);
    sigint = wl_event_loop_add_signal(loop, SIGINT, handle_sigint, display);
    PEPPER_ASSERT(sigint);

    wl_display_run(display);

    wl_event_source_remove(sigint);
    pepper_drm_destroy(drm);
    pepper_compositor_destroy(compositor);

    return 0;
}
