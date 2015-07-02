#include "desktop-shell-internal.h"
#include <stdlib.h>

shell_client_t *
shell_client_create(desktop_shell_t *shell, struct wl_client *client,
                    const struct wl_interface *interface, const void *implementation,
                    uint32_t version, uint32_t id)
{
    shell_client_t  *shell_client;

    shell_client = calloc(1, sizeof(shell_client_t));
    if (!shell_client)
    {
        wl_client_post_no_memory(client);
        return NULL;
    }

    shell_client->resource = wl_resource_create(client, interface, version, id);
    if (!shell_client->resource)
    {
        wl_client_post_no_memory(client);
        free(shell_client);
        return NULL;
    }
    wl_resource_set_implementation(shell_client->resource, implementation, shell_client, NULL);

    wl_list_insert(&shell->shell_client_list, &shell_client->link);

    shell_client->shell = shell;

    return shell_client;
}

PEPPER_API pepper_bool_t
pepper_desktop_shell_init(pepper_object_t *compositor)
{
    desktop_shell_t *shell;

    shell = calloc(1, sizeof(desktop_shell_t));
    if (!shell)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return PEPPER_FALSE;
    }

    shell->compositor = compositor;

    wl_list_init(&shell->shell_client_list);
    wl_list_init(&shell->shell_surface_list);

    if (!init_wl_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        free(shell);
        return PEPPER_FALSE;
    }

    return PEPPER_TRUE;
}
