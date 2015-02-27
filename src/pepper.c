#include <stdio.h>
#include <stdlib.h>

#include <wayland-server.h>

int
main(int argc, char **argv)
{
    int			ret = EXIT_SUCCESS;
    struct wl_display *	display = NULL;
    const char *		socket_name = NULL;

    /* Create a display object. */
    display = wl_display_create();

    if (!display)
    {
	printf("Failed to create display.\n");
	ret = EXIT_FAILURE;
	goto out;
    }

    /* Add a listening socket. */
    socket_name = wl_display_add_socket_auto(display);

    if (!socket_name)
    {
	printf("Failed to add socket.\n");
	ret = EXIT_FAILURE;
	goto out;
    }

    printf("Pepper server socket added: %s\n", socket_name);

    /* Start main loop. */
    wl_display_run(display);

out:
    if (display)
	wl_display_destroy(display);

    return ret;
}
