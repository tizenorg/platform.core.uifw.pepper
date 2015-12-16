/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <libudev.h>

#include <config.h>
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
    int                     tty;
    const char             *tty_str;

    init_signals();

    tty_str = getenv("PEPPER_DRM_TTY");

    if (!tty_str)
        tty = 0;
    else
        tty = atoi(tty_str);

    if (!pepper_virtual_terminal_setup(tty))
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

    drm = pepper_drm_create(compositor, udev, NULL);
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
