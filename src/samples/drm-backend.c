#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <libudev.h>

#include <pepper.h>
#include <pepper-libinput.h>
#include <pepper-drm.h>
#include <pepper-desktop-shell.h>

static void
handle_signals(int s, siginfo_t *siginfo, void *context)
{
    pepper_virtual_terminal_restore();
    raise(SIGTRAP);
}

static void
init_signals()
{
    struct sigaction action;

    action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    action.sa_sigaction = handle_signals;
    sigemptyset(&action.sa_mask);

    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGABRT, &action, NULL);
}

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
    pepper_compositor_t    *compositor = NULL;
    pepper_drm_t           *drm = NULL;
    pepper_libinput_t      *input = NULL;

    struct udev            *udev = NULL;

    struct wl_display      *display = NULL;
    struct wl_event_loop   *loop = NULL;
    struct wl_event_source *sigint = NULL;

    {   /* for gdb attach */
        char cc;
        int  ret;

        ret = scanf("%c", &cc);
    }

    init_signals();

    if (!pepper_virtual_terminal_setup(0/*FIXME*/))
        goto cleanup;

    compositor = pepper_compositor_create("wayland-0");
    if (!compositor)
        goto cleanup;

    udev = udev_new();
    if (!udev)
        goto cleanup;

    input = pepper_libinput_create(compositor, udev);
    if (!input)
        goto cleanup;

    drm = pepper_drm_create(compositor, udev, ""/*device*/, "pixman"/*renderer*/);
    if (!drm)
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

    if (drm)
        pepper_drm_destroy(drm);

    if (input)
        pepper_libinput_destroy(input);

    if (udev)
        udev_unref(udev);

    if (compositor)
        pepper_compositor_destroy(compositor);

    pepper_virtual_terminal_restore();

    return 0;
}
