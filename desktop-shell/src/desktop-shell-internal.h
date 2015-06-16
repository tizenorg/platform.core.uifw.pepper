#include <config.h>
#include "pepper-desktop-shell.h"
#include <wayland-server.h>

/* TODO: */
#define PEPPER_ERROR(...)

/* Ping timeout value in ms. */
#define DESKTOP_SHELL_PING_TIMEOUT  200

typedef struct shell            shell_t;
typedef struct shell_surface    shell_surface_t;

struct shell
{
    pepper_object_t    *compositor;
    struct wl_resource *resource;
};

struct shell_surface
{
    shell_t                *shell;

    struct wl_client       *client;
    struct wl_resource     *resource;

    struct wl_listener      client_destroy_listener;
    struct wl_listener      surface_destroy_listener;

    struct wl_event_source *ping_timer;
    pepper_bool_t           need_pong;
};

shell_t *
shell_create(pepper_object_t *compositor, struct wl_client *client,
             const struct wl_interface *interface, const void *implementation,
             uint32_t version, uint32_t id);

shell_surface_t *
shell_surface_create(shell_t *shell, pepper_object_t *surface, struct wl_client *client,
                     const char *role_name, const struct wl_interface *interface,
                     const void *implemenetation, uint32_t version, uint32_t id);

void
shell_surface_ping(shell_surface_t *shsurf);

pepper_bool_t
init_wl_shell(pepper_object_t *compositor);
