#include "desktop-shell-internal.h"
#include <stdlib.h>

shell_t *
shell_create(pepper_compositor_t *compositor, struct wl_client *client,
             const struct wl_interface *interface, const void *implementation,
             uint32_t version, uint32_t id)
{
    shell_t *shell;

    shell = calloc(1, sizeof(shell_t));
    if (!shell)
    {
        wl_client_post_no_memory(client);
        return NULL;
    }

    shell->resource = wl_resource_create(client, interface, version, id);
    if (!shell->resource)
    {
        wl_client_post_no_memory(client);
        pepper_free(shell);
        return NULL;
    }

    wl_resource_set_implementation(shell->resource, implementation, shell, NULL);

    shell->compositor = compositor;
    return shell;
}

PEPPER_API pepper_bool_t
pepper_desktop_shell_init(pepper_compositor_t *compositor)
{
    if (!init_wl_shell(compositor))
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}
