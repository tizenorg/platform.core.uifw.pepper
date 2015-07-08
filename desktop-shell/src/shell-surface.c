#include "desktop-shell-internal.h"
#include "xdg-shell-server-protocol.h"

void
remove_ping_timer(shell_client_t *shell_client)
{
    if (shell_client->ping_timer)
    {
        wl_event_source_remove(shell_client->ping_timer);
        shell_client->ping_timer = NULL;
    }
}

static void
shsurf_stop_listen_commit_event(shell_surface_t *shsurf)
{
    wl_list_remove(&shsurf->surface_commit_listener.link);
    wl_list_init(&shsurf->surface_commit_listener.link);
}

static void
handle_surface_commit(struct wl_listener *listener, void *data)
{
    shell_surface_t *shsurf =
        pepper_container_of(listener, shell_surface_t, surface_commit_listener);

    if (!shsurf->mapped && shsurf->shell_surface_map)
        shsurf->shell_surface_map(shsurf);
}

static void
shsurf_start_listen_commit_event(shell_surface_t *shsurf)
{
    shsurf->surface_commit_listener.notify = handle_surface_commit;
    wl_list_init(&shsurf->surface_commit_listener.link);
    /* TODO: Need new API: pepper_surface_add_commit_listener() */
}

static void
handle_client_destroy(struct wl_listener *listener, void *data)
{
    shell_surface_t *shsurf =
        pepper_container_of(listener, shell_surface_t, client_destroy_listener);

    if (!wl_list_empty(&shsurf->client_destroy_listener.link))
        wl_list_remove(&shsurf->client_destroy_listener.link);
    wl_list_init(&shsurf->client_destroy_listener.link);
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

    shsurf_stop_listen_commit_event(shsurf);

    if (shsurf->resource)
        wl_resource_destroy(shsurf->resource);

    if (shsurf->title)
        free(shsurf->title);

    if (shsurf->class_)
        free(shsurf->class_);

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
shell_surface_create(shell_client_t *shell_client, pepper_object_t *surface,
                     struct wl_client *client, const struct wl_interface *interface,
                     const void *implementation, uint32_t version, uint32_t id)
{
    shell_surface_t     *shsurf = NULL;

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

    shsurf->shell_client = shell_client;
    shsurf->shell        = shell_client->shell;
    shsurf->client       = client;
    shsurf->surface      = surface;

    shsurf->view    = pepper_compositor_add_surface_view(shell_client->shell->compositor, surface);
    if (!shsurf->view)
    {
        PEPPER_ERROR("pepper_compositor_add_view failed\n");
        goto error;
    }

    wl_list_init(&shsurf->child_list);
    wl_list_init(&shsurf->parent_link);

    wl_resource_set_implementation(shsurf->resource, implementation, shsurf, handle_resource_destroy);

    shsurf->client_destroy_listener.notify = handle_client_destroy;
    wl_client_add_destroy_listener(client, &shsurf->client_destroy_listener);

    shsurf->surface_destroy_listener.notify = handle_surface_destroy;
    pepper_object_add_destroy_listener(surface, &shsurf->surface_destroy_listener);

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_NONE);
    shsurf_start_listen_commit_event(shsurf);

    /* Set shell_surface_t to pepper_surface_t */
    set_shsurf_to_surface(surface, shsurf);

    wl_list_init(&shsurf->link);
    wl_list_insert(&shsurf->shell->shell_surface_list, &shsurf->link);

    return shsurf;

error:
    if (shsurf)
        free(shsurf);

    return NULL;
}

static int
handle_ping_timeout(void *data)
{
    shell_client_t *shell_client = data;

    shell_client->irresponsive = PEPPER_TRUE;

    /* TODO: Display wait cursor */

    return 1;
}

void
shell_surface_ping(shell_surface_t *shsurf)
{
    shell_client_t      *shell_client = shsurf->shell_client;
    struct wl_display   *display;
    const char          *role;

    /* Already stucked, do not send another ping */
    if (shell_client->irresponsive)
    {
        handle_ping_timeout(shell_client);
        return ;
    }

    display = pepper_compositor_get_display(shsurf->shell->compositor);

    if (!shell_client->ping_timer)
    {
        struct wl_event_loop *loop = wl_display_get_event_loop(display);

        shell_client->ping_timer = wl_event_loop_add_timer(loop, handle_ping_timeout, shell_client);

        if (!shell_client->ping_timer)
        {
            PEPPER_ERROR("Failed to add timer event source.\n");
            return;
        }
    }

    wl_event_source_timer_update(shell_client->ping_timer, DESKTOP_SHELL_PING_TIMEOUT);

    shell_client->ping_serial = wl_display_next_serial(display);
    shell_client->need_pong   = PEPPER_TRUE;

    role = pepper_surface_get_role(shsurf->surface);

    if (!strcmp(role, "wl_shell_surface"))
        wl_shell_surface_send_ping(shsurf->resource, shell_client->ping_serial);
    else if (!strcmp(role, "xdg_surface") || !strcmp(role, "xdg_popup"))
        xdg_shell_send_ping(shell_client->resource, shell_client->ping_serial);
    else
        PEPPER_ASSERT(0);
}

void
shell_client_handle_pong(shell_client_t *shell_client, uint32_t serial)
{
    /* Client response right ping_serial */
    if (shell_client->need_pong && shell_client->ping_serial == serial)
    {
        wl_event_source_timer_update(shell_client->ping_timer, 0);    /* disarms the timer */

        shell_client->irresponsive = PEPPER_FALSE;
        shell_client->need_pong    = PEPPER_FALSE;
        shell_client->ping_serial  = 0;

        /* TODO: Stop displaying wait cursor */
    }
}

void
shell_surface_handle_pong(shell_surface_t *shsurf, uint32_t serial)
{
    shell_client_handle_pong(shsurf->shell_client, serial);
}

void
shell_surface_set_toplevel(shell_surface_t *shsurf)
{
    shell_surface_set_parent(shsurf, NULL);

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_TOPLEVEL);

    /* Need to map in later */
    shsurf->mapped = PEPPER_FALSE;
}

static void
shell_surface_set_position(shell_surface_t *shsurf, int32_t x, int32_t y)
{
    pepper_view_set_position(shsurf->view, x, y);
}

static void
shell_surface_map_toplevel(shell_surface_t *shsurf)
{
    int32_t x = 0, y = 0;

    /**
     * TODO: To placing view, need to get output's size, position and seat->pointer's position
     *       or, read from config file
     */

    shell_surface_set_position(shsurf, x, y);

    pepper_view_map(shsurf->view);

    shsurf->mapped = PEPPER_TRUE;
}

void
shell_surface_set_type(shell_surface_t *shsurf, shell_surface_type_t type)
{
    shsurf->type = type;

    switch (type)
    {
    case SHELL_SURFACE_TYPE_NONE:
        shsurf->shell_surface_map = NULL;
        break;
    case SHELL_SURFACE_TYPE_TOPLEVEL:
        shsurf->shell_surface_map = shell_surface_map_toplevel;
        break;
    default :
        /* XXX: Maybe some logs be needed */
        break;
    }
}

shell_surface_t *
get_shsurf_from_surface(pepper_object_t *surface, desktop_shell_t *shell)
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
