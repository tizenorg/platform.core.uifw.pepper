#include <config.h>
#include "pepper-desktop-shell.h"
#include <wayland-server.h>

/* TODO: */
#define PEPPER_ERROR(...)
#define PEPPER_ASSERT(...)

/* Ping timeout value in ms. */
#define DESKTOP_SHELL_PING_TIMEOUT  200

typedef struct desktop_shell    desktop_shell_t;
typedef struct shell_client     shell_client_t;
typedef struct shell_surface    shell_surface_t;


struct desktop_shell
{
    pepper_object_t         *compositor;

    struct wl_list           shell_client_list;
    struct wl_list           shell_surface_list;

    /* TODO: */
    struct wl_listener       seat_create_listener;
    struct wl_listener       output_create_listener;
    struct wl_listener       output_change_listener;
};

struct shell_client
{
    desktop_shell_t         *shell;
    struct wl_resource      *resource;
    struct wl_client        *client;

    struct wl_listener       client_destroy_listener;

    /* Ping-Pong */
    struct wl_event_source  *ping_timer;
    pepper_bool_t            need_pong;
    uint32_t                 ping_serial;
    pepper_bool_t            irresponsive;

    struct wl_list           link;
};

typedef enum
{
    SHELL_SURFACE_TYPE_NONE,
    SHELL_SURFACE_TYPE_TOPLEVEL,
    SHELL_SURFACE_TYPE_TRANSIENT,
    SHELL_SURFACE_TYPE_FULLSCREEN,
    SHELL_SURFACE_TYPE_POPUP,
    SHELL_SURFACE_TYPE_MAXIMIZED
} shell_surface_type_t;

struct shell_surface
{
    desktop_shell_t         *shell;

    shell_client_t          *shell_client;

    struct wl_client        *client;
    struct wl_resource      *resource;

    /* Hierarchy */
    pepper_object_t         *parent;
    struct wl_list           child_list;   /* children surfaces of this */
    struct wl_list           parent_link;

    /* Contents */
    pepper_object_t         *surface;
    pepper_object_t         *view;

    char                    *title, *class_;

    /* Data structures per surface type */
    shell_surface_type_t     type;

    /* (*map) */
    void (*shell_surface_map)(shell_surface_t *shsurf);
    pepper_bool_t            mapped;

    /* Listeners */
    struct wl_listener      client_destroy_listener;
    struct wl_listener      surface_destroy_listener;
    struct wl_listener      surface_commit_listener;

    struct wl_list          link;       /* link */
};

shell_client_t *
shell_client_create(desktop_shell_t *shell, struct wl_client *client,
             const struct wl_interface *interface, const void *implementation,
             uint32_t version, uint32_t id);

shell_surface_t *
shell_surface_create(shell_client_t *shell, pepper_object_t *surface, struct wl_client *client,
                     const struct wl_interface *interface,
                     const void *implemenetation, uint32_t version, uint32_t id);

void
shell_surface_ping(shell_surface_t *shsurf);

void
shell_client_handle_pong(shell_client_t *shell_client, uint32_t serial);

void
shell_surface_handle_pong(shell_surface_t *shsurf, uint32_t serial);

void
remove_ping_timer(shell_client_t *shell_client);

void
shell_surface_set_type(shell_surface_t *shsurf, shell_surface_type_t type);

void
shell_surface_set_parent(shell_surface_t *shsurf, pepper_object_t *parent_surface);

/* */
shell_surface_t *
get_shsurf_from_surface(pepper_object_t *surface, desktop_shell_t *shell);

void
set_shsurf_to_surface(pepper_object_t *surface, shell_surface_t *shsurf);

void
shell_surface_set_toplevel(shell_surface_t *shsurf);

pepper_bool_t
init_wl_shell(desktop_shell_t *shell);
