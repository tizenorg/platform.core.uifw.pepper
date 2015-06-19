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
    shell_surface_t *shsurf =
        pepper_container_of(listener, shell_surface_t, client_destroy_listener);

    if (!wl_list_empty(&shsurf->client_destroy_listener.link))
        wl_list_remove(&shsurf->client_destroy_listener.link);
    wl_list_init(&shsurf->client_destroy_listener.link);

    remove_ping_timer(shsurf);

    /* client_destroy -> client's surface destroy -> handle_surface_destroy */
}

static void
handle_surface_destroy(struct wl_listener *listener, void *data)
{
    shell_surface_t *child, *tmp;
    shell_surface_t *shsurf =
        pepper_container_of(listener, shell_surface_t, surface_destroy_listener);

    if (!wl_list_empty(&shsurf->client_destroy_listener.link))
        wl_list_remove(&shsurf->client_destroy_listener.link);

    wl_list_remove(&shsurf->surface_destroy_listener.link);

    /* We don't need to destroy pepper_view_t */

    if (shsurf->resource)
        wl_resource_destroy(shsurf->resource);

    if (shsurf->title)
        free(shsurf->title);

    if (shsurf->class_)
        free(shsurf->class_);

    remove_ping_timer(shsurf);

    wl_list_remove(&shsurf->parent_link);

    wl_list_for_each_safe(child, tmp, &shsurf->child_list, parent_link)
        shell_surface_set_parent(child, NULL);

    wl_list_remove(&shsurf->link);
    free(shsurf);
}

static void
handle_resource_destroy(struct wl_resource *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shsurf->resource = NULL;
}

shell_surface_t *
shell_surface_create(shell_t *shell, pepper_object_t *surface, struct wl_client *client,
                     const struct wl_interface *interface,
                     const void *implementation, uint32_t version, uint32_t id)
{
    shell_surface_t *shsurf = NULL;

    shsurf = calloc(1, sizeof(shell_surface_t));
    if (!shsurf)
    {
        PEPPER_ERROR("Memory allocation faiiled\n");
        goto error;
    }

    shsurf->resource = wl_resource_create(client, interface, version, id);
    if (!shsurf->resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        goto error;
    }

    shsurf->shell   = shell;
    shsurf->client  = client;
    shsurf->surface = surface;
    shsurf->view    = pepper_compositor_add_view(shell->compositor, NULL, NULL, surface);
    if (!shsurf->view)
    {
        PEPPER_ERROR("pepper_compositor_add_view failed\n");
        goto error;
    }

    /* TODO: Need to know about output size */
    shsurf->geometry.x = rand()%10;
    shsurf->geometry.y = rand()%10;
    pepper_view_set_position(shsurf->view, shsurf->geometry.x, shsurf->geometry.y);

    wl_list_init(&shsurf->link);
    wl_list_insert(&shell->shell_surface_list, &shsurf->link);

    wl_list_init(&shsurf->child_list);
    wl_list_init(&shsurf->parent_link);

    /* Set shell_surface_t to pepper_surface_t */
    set_shsurf_to_surface(surface, shsurf);

    wl_resource_set_implementation(shsurf->resource, implementation, shsurf, handle_resource_destroy);

    shsurf->client_destroy_listener.notify = handle_client_destroy;
    wl_client_add_destroy_listener(client, &shsurf->client_destroy_listener);

    shsurf->surface_destroy_listener.notify = handle_surface_destroy;
    pepper_object_add_destroy_listener(surface, &shsurf->surface_destroy_listener);

    return shsurf;

error:
    if (shsurf)
        free(shsurf);

    wl_client_post_no_memory(client);
    return NULL;
}

static int
handle_ping_timeout(void *data)
{
    shell_surface_t *shsurf = data;

    shsurf->unresponsive = PEPPER_TRUE;

    /* TODO: Display wait cursor */

    return 1;
}

void
shell_surface_ping(shell_surface_t *shsurf)
{
    struct wl_display *display = pepper_compositor_get_display(shsurf->shell->compositor);
    const char *role;

    /* Already stucked, do not send another ping */
    if (shsurf->unresponsive)
    {
        handle_ping_timeout(shsurf);
        return ;
    }

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

    shsurf->ping_serial = wl_display_next_serial(display);
    shsurf->need_pong   = PEPPER_TRUE;

    role = pepper_surface_get_role(shsurf->surface);

    if (!strcmp(role, "wl_shell_surface"))
        wl_shell_surface_send_ping(shsurf->resource, shsurf->ping_serial);

    /* TODO: Do another protocol specific ping. */
}

void
shell_surface_set_type(shell_surface_t *shsurf, enum shell_surface_type type)
{
    shsurf->type = type;
}

shell_surface_t *
get_shsurf_from_surface(pepper_object_t *surface, shell_t *shell)
{
    return pepper_object_get_user_data(surface, shell);
}

void
set_shsurf_to_surface(pepper_object_t *surface, shell_surface_t *shsurf)
{
    pepper_object_set_user_data(surface, shsurf->shell, shsurf, NULL);
}

void
shell_surface_set_parent(shell_surface_t *shsurf, pepper_object_t *parent)
{
    shell_surface_t *parent_shsurf;

    wl_list_remove(&shsurf->parent_link);
    wl_list_init(&shsurf->parent_link);

    shsurf->parent = parent;

    if (parent)
    {
        parent_shsurf = get_shsurf_from_surface(parent, shsurf->shell);
        if (parent_shsurf)
            wl_list_insert(&parent_shsurf->child_list, &shsurf->parent_link);
    }
}
