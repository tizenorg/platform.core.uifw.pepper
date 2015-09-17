#include "desktop-shell-internal.h"
#include <stdlib.h>

void
shell_get_output_workarea(desktop_shell_t       *shell,
                          pepper_output_t       *output,
                          pixman_rectangle32_t  *area)
{
    const pepper_output_geometry_t *geom;

    /**
     ** TODO: Get given output's workarea size and position in global coordinate
     **      return (output_size - (panel_size + margin + caption + ... ));
     **/

    geom = pepper_output_get_geometry(output);

    if (area)
    {
        area->x = geom->x;
        area->y = geom->y;
        area->width = geom->w;
        area->height = geom->h;
    }
}

static void
handle_shell_client_destroy(struct wl_listener *listener, void *data)
{
    shell_client_t *shell_client = pepper_container_of(listener, shell_client,
                                                       client_destroy_listener);

    remove_ping_timer(shell_client);
    pepper_list_remove(&shell_client->link);
    free(shell_client);
}

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

    shell_client->shell  = shell;
    shell_client->client = client;

    shell_client->client_destroy_listener.notify = handle_shell_client_destroy;
    wl_client_add_destroy_listener(client, &shell_client->client_destroy_listener);

    pepper_list_insert(&shell->shell_client_list, &shell_client->link);
    wl_resource_set_implementation(shell_client->resource, implementation, shell_client, NULL);

    return shell_client;
}

static void
shell_add_input_device(desktop_shell_t *shell, pepper_input_device_t *device)
{
    shell_seat_t            *shseat;
    pepper_seat_t           *seat;
    const char              *target_seat_name;
    const char              *seat_name;

    target_seat_name = pepper_input_device_get_property(device, "seat_name");
    if (!target_seat_name)
        target_seat_name = "seat0";

    pepper_list_for_each(shseat, &shell->shseat_list, link)
    {
        seat_name = pepper_seat_get_name(shseat->seat);

        /* Find seat to adding input device */
        if ( seat_name && !strcmp(seat_name, target_seat_name))
        {
            pepper_seat_add_input_device(shseat->seat, device);
            return ;
        }
    }

    seat = pepper_compositor_add_seat(shell->compositor, target_seat_name);
    pepper_seat_add_input_device(seat, device);
}

static void
shell_add_seat(desktop_shell_t *shell, pepper_seat_t *seat)
{
    shell_seat_t            *shseat;

    pepper_list_for_each(shseat, &shell->shseat_list, link)
    {
        if (shseat->seat == seat)
            return ;
    }

    shseat = calloc(1, sizeof(shell_seat_t));
    if (!shseat)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return ;
    }

    shseat->seat  = seat;
    shseat->shell = shell;

    pepper_list_insert(&shell->shseat_list, &shseat->link);
    pepper_object_set_user_data((pepper_object_t *)seat, shell, shseat, NULL);
}

static void
shell_remove_seat(desktop_shell_t *shell, pepper_seat_t *seat)
{
    shell_seat_t            *shseat;

    pepper_list_for_each(shseat, &shell->shseat_list, link)
    {
        if (shseat->seat == seat)
        {
            pepper_list_remove(&shseat->link);
            free(shseat);
            return ;
        }
    }
}

static void
input_device_add_callback(pepper_event_listener_t    *listener,
                          pepper_object_t            *object,
                          uint32_t                    id,
                          void                       *info,
                          void                       *data)
{
    shell_add_input_device(data, info);
}

static void
seat_add_callback(pepper_event_listener_t    *listener,
                  pepper_object_t            *object,
                  uint32_t                    id,
                  void                       *info,
                  void                       *data)
{
    shell_add_seat(data, info);
}

static void
seat_remove_callback(pepper_event_listener_t    *listener,
                     pepper_object_t            *object,
                     uint32_t                    id,
                     void                       *info,
                     void                       *data)
{
    shell_remove_seat(data, info);
}

static void
init_listeners(desktop_shell_t *shell)
{
    pepper_object_t *compositor = (pepper_object_t *)shell->compositor;

    /* input_device_add */
    shell->input_device_add_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
                                         0, input_device_add_callback, shell);

    shell->seat_add_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
                                         0, seat_add_callback, shell);

    shell->seat_remove_listener =
        pepper_object_add_event_listener(compositor, PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
                                         0, seat_remove_callback, shell);
}

static void
init_input(desktop_shell_t *shell)
{
    pepper_list_t *l;
    const pepper_list_t *input_device_list =
        pepper_compositor_get_input_device_list(shell->compositor);

    pepper_list_for_each_list(l, input_device_list)
        shell_add_input_device(shell, l->item);
}

PEPPER_API pepper_bool_t
pepper_desktop_shell_init(pepper_compositor_t *compositor)
{
    desktop_shell_t *shell;

    shell = calloc(1, sizeof(desktop_shell_t));
    if (!shell)
    {
        PEPPER_ERROR("Memory allocation failed\n");
        return PEPPER_FALSE;
    }

    shell->compositor = compositor;

    pepper_list_init(&shell->shell_client_list);
    pepper_list_init(&shell->shell_surface_list);
    pepper_list_init(&shell->shseat_list);

    if (!init_wl_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        free(shell);
        return PEPPER_FALSE;
    }

    if (!init_xdg_shell(shell))
    {
        PEPPER_ERROR("wl_shell initialize failed\n");
        fini_wl_shell(shell);
        free(shell);
        return PEPPER_FALSE;
    }

    init_listeners(shell);
    init_input(shell);

    return PEPPER_TRUE;
}
