#include "wayland-internal.h"

#define WAYLAND_DATA_KEY    0x721dfa02

static void
handle_global(void *data, struct wl_registry *registry,
              uint32_t name, const char *interface, uint32_t version)
{
    wayland_connection_t *conn = data;

    if (strcmp(interface, "wl_compositor") == 0)
    {
        conn->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
    }
    else if (strcmp(interface, "wl_seat") == 0)
    {
        wayland_handle_global_seat(conn, registry, name, version);
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        conn->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
    }
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    PEPPER_TRACE("TODO: %s\n", __FUNCTION__);
}

static const struct wl_registry_listener registry_listener =
{
    handle_global,
    handle_global_remove,
};

static int
handle_wayland_event(int fd, uint32_t mask, void *data)
{
    wayland_connection_t *conn = data;
    int count = 0;

    if ((mask & WL_EVENT_HANGUP) || (mask & WL_EVENT_ERROR))
        return 0;

    if (mask & WL_EVENT_READABLE)
        count = wl_display_dispatch(conn->display);

    if (mask & WL_EVENT_WRITABLE)
        wl_display_flush(conn->display);

    if (mask == 0)
    {
        count = wl_display_dispatch_pending(conn->display);
        wl_display_flush(conn->display);
    }

    return count;
}

wayland_connection_t *
wayland_get_connection(pepper_compositor_t *compositor, const char *socket_name)
{
    wayland_data_t          *data;
    wayland_connection_t    *conn = NULL;

    data = pepper_compositor_get_user_data(compositor, WAYLAND_DATA_KEY);
    if (!data)
    {
        PEPPER_ERROR("Wayland module is not initialized. Call pepper_wayland_init() first.\n");
        return NULL;
    }

    if (!wl_list_empty(&data->connections))
    {
        wayland_connection_t *c;
        wl_list_for_each(c, &data->connections, link)
        {
            if (strcmp(c->socket_name, socket_name) == 0)
            {
                conn = c;
                break;
            }
        }
    }

    if (!conn)
    {
        struct wl_display       *display;
        struct wl_event_loop    *loop;

        conn = (wayland_connection_t *)pepper_calloc(1, sizeof(wayland_connection_t));
        if (!conn)
            return NULL;

        conn->data = data;
        conn->display = wl_display_connect(socket_name);
        conn->fd = wl_display_get_fd(conn->display);
        conn->socket_name = pepper_string_copy(socket_name);

        display = pepper_compositor_get_display(compositor);
        loop = wl_display_get_event_loop(display);

        conn->event_source = wl_event_loop_add_fd(loop, conn->fd, WL_EVENT_READABLE,
                                                  handle_wayland_event, conn);

        conn->registry = wl_display_get_registry(conn->display);
        wl_registry_add_listener(conn->registry, &registry_listener, conn);
        wl_display_roundtrip(conn->display);

        wl_list_insert(&data->connections, &conn->link);
    }

    return conn;
}

PEPPER_API pepper_bool_t
pepper_wayland_init(pepper_compositor_t *compositor)
{
    wayland_data_t *data = pepper_compositor_get_user_data(compositor, WAYLAND_DATA_KEY);

    if (data)
    {
        PEPPER_ERROR("Wayland key is already used by another module.\n");
        return PEPPER_FALSE;
    }

    data = (wayland_data_t *)pepper_calloc(1, sizeof(wayland_data_t));
    if (!data)
        return PEPPER_FALSE;

    data->compositor = compositor;
    wl_list_init(&data->connections);
    pepper_compositor_set_user_data(compositor, WAYLAND_DATA_KEY, data);

    return PEPPER_TRUE;
}
