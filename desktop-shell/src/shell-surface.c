#include "desktop-shell-internal.h"
#include <stdlib.h>

static void
remove_ping_timer(shell_surface_t *shsurf)
{
    if (shsurf->ping_timer)
    {
        wl_event_source_remove(shsurf->ping_timer);
        shsurf->ping_timer = NULL;
    }
}

static void
handle_client_destroy(struct wl_listener *listener, void *data)
{
    shell_surface_t *shsurf = wl_container_of(listener, shsurf, client_destroy_listener);

    wl_list_remove(&shsurf->client_destroy_listener.link);
    wl_list_init(&shsurf->client_destroy_listener.link);

    remove_ping_timer(shsurf);
}

static void
handle_surface_destroy(struct wl_listener *listener, void *data)
{
    shell_surface_t *shsurf = wl_container_of(listener, shsurf, surface_destroy_listener);

    if (!wl_list_empty(&shsurf->client_destroy_listener.link))
        wl_list_remove(&shsurf->client_destroy_listener.link);

    wl_list_remove(&shsurf->surface_destroy_listener.link);

    if (shsurf->resource)
        wl_resource_destroy(shsurf->resource);

    remove_ping_timer(shsurf);
    pepper_free(shsurf);
}

static void
handle_resource_destroy(struct wl_resource *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);
    shsurf->resource = NULL;
}

shell_surface_t *
shell_surface_create(shell_t *shell, pepper_surface_t *surface, struct wl_client *client,
                     const char *role_name, const struct wl_interface *interface,
                     const void *implementation, uint32_t version, uint32_t id)
{
    shell_surface_t *shsurf;

    if (pepper_surface_get_role(surface))
        return NULL;

    shsurf = calloc(1, sizeof(shell_surface_t));
    if (!shsurf)
        return NULL;

    shsurf->resource = wl_resource_create(client, interface, version, id);
    if (!shsurf->resource)
    {
        pepper_free(shsurf);
        return NULL;
    }

    shsurf->shell = shell;
    shsurf->client = client;

    wl_resource_set_implementation(shsurf->resource, implementation, shsurf, handle_resource_destroy);

    shsurf->client_destroy_listener.notify = handle_client_destroy;
    wl_client_add_destroy_listener(client, &shsurf->client_destroy_listener);

    shsurf->surface_destroy_listener.notify = handle_surface_destroy;
    pepper_surface_add_destroy_listener(surface, &shsurf->surface_destroy_listener);

    return shsurf;
}

static int
handle_ping_timeout(void *data)
{
    /* TODO: */
    return 1;
}

void
shell_surface_ping(shell_surface_t *shsurf)
{
    struct wl_display *display = pepper_compositor_get_display(shsurf->shell->compositor);

    /* TODO: Check if the backend support ping. */

    if (!shsurf->ping_timer)
    {
        struct wl_event_loop *loop = wl_display_get_event_loop(display);

        shsurf->ping_timer = wl_event_loop_add_timer(loop, handle_ping_timeout, shsurf);

        if (!shsurf->ping_timer)
        {
            PEPPER_ERROR("Failed to add timer event source.\n");
            return;
        }
    }

    wl_event_source_timer_update(shsurf->ping_timer, DESKTOP_SHELL_PING_TIMEOUT);
    shsurf->need_pong = PEPPER_TRUE;

    /* TODO: Do protocol specific ping. */
}
