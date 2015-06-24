#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <pepper.h>
#include <pepper-fbdev.h>
#include <pepper-desktop-shell.h>

/* TODO: */
#define PEPPER_ASSERT(exp)
#define PEPPER_ERROR(...)

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
    pepper_object_t        *compositor;
    pepper_fbdev_t         *fbdev;
    struct wl_display      *display;
    struct wl_event_loop   *loop;
    struct wl_event_source *sigint;

    {   /* for gdb attach */
        char cc;
        int  ret;

        ret = scanf("%c", &cc);
        if (ret < 0)
            return -1;
    }

    if (!pepper_virtual_terminal_setup(0/*FIXME*/))
        goto cleanup;

    compositor = pepper_compositor_create("wayland-0");
    if (!compositor)
        goto cleanup;

    fbdev = pepper_fbdev_create(compositor, "", "pixman");
    if (!fbdev)
        goto cleanup;

    if (!pepper_desktop_shell_init(compositor))
        goto cleanup;

    display = pepper_compositor_get_display(compositor);
    if (!display)
        goto cleanup;

    loop = wl_display_get_event_loop(display);
    sigint = wl_event_loop_add_signal(loop, SIGINT, handle_sigint, display);
    if (!sigint)
        goto cleanup;

    wl_display_run(display);

cleanup:

    if (sigint)
        wl_event_source_remove(sigint);

    if (fbdev)
        pepper_fbdev_destroy(fbdev);

    if (compositor)
        pepper_compositor_destroy(compositor);

    pepper_virtual_terminal_restore();

    return 0;
}
