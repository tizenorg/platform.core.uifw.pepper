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
    struct wl_list      shell_surface_list;
};

enum shell_surface_type
{
    SHELL_SURFACE_TYPE_NONE,
    SHELL_SURFACE_TYPE_TOPLEVEL,
    SHELL_SURFACE_TYPE_FULLSCREEN,
    SHELL_SURFACE_TYPE_POPUP,
    SHELL_SURFACE_TYPE_MAXIMIZED
};

struct shell_surface
{
    shell_t                *shell;

    struct wl_client       *client;
    struct wl_resource     *resource;

    pepper_object_t        *parent;
    struct wl_list          child_list;   /* children surfaces of this */
    struct wl_list          parent_link;

    pepper_object_t        *view;

    enum shell_surface_type type;

    char                   *title, *class_;

    struct
    {
        int32_t x, y, width, height;
    } geometry, saved;

    struct wl_event_source *ping_timer;
    pepper_bool_t           need_pong;

    struct wl_listener      client_destroy_listener;
    struct wl_listener      surface_destroy_listener;

    struct wl_list          link;       /* link */
};

shell_t *
shell_create(pepper_object_t *compositor, struct wl_client *client,
             const struct wl_interface *interface, const void *implementation,
             uint32_t version, uint32_t id);

shell_surface_t *
shell_surface_create(shell_t *shell, pepper_object_t *surface, struct wl_client *client,
                     const struct wl_interface *interface,
                     const void *implemenetation, uint32_t version, uint32_t id);

void
shell_surface_ping(shell_surface_t *shsurf);

void
shell_surface_set_type(shell_surface_t *shsurf, enum shell_surface_type type);

void
shell_surface_set_parent(shell_surface_t *shsurf, pepper_object_t *parent_surface);

/* */
shell_surface_t *
get_shsurf_from_surface(pepper_object_t *surface, shell_t *shell);

void
set_shsurf_to_surface(pepper_object_t *surface, shell_surface_t *shsurf);

pepper_bool_t
init_wl_shell(pepper_object_t *compositor);
