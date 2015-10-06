#include <config.h>
#include "pepper-desktop-shell.h"
#include <wayland-server.h>

/* Ping timeout value in ms. */
#define DESKTOP_SHELL_PING_TIMEOUT  200

typedef struct desktop_shell    desktop_shell_t;
typedef struct shell_client     shell_client_t;
typedef struct shell_surface    shell_surface_t;
typedef struct shell_seat       shell_seat_t;

struct shell_seat
{
    desktop_shell_t             *shell;
    pepper_seat_t               *seat;
    pepper_list_t                link;

    /* Seat's logical device add/remove */
    pepper_event_listener_t     *pointer_add_listener;
    pepper_event_listener_t     *pointer_remove_listener;

    pepper_event_listener_t     *keyboard_add_listener;
    pepper_event_listener_t     *keyboard_remove_listener;

    pepper_event_listener_t     *touch_add_listener;
    pepper_event_listener_t     *touch_remove_listener;
};

struct desktop_shell
{
    pepper_compositor_t         *compositor;

    pepper_list_t                shell_client_list;
    pepper_list_t                shell_surface_list;
    pepper_list_t                shseat_list;

    /* input device add/remove listeners */
    pepper_event_listener_t     *input_device_add_listener;

    /* seat add/remove */
    pepper_event_listener_t     *seat_add_listener;
    pepper_event_listener_t     *seat_remove_listener;
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

    pepper_list_t            link;
};

typedef enum
{
    SHELL_SURFACE_TYPE_NONE,
    SHELL_SURFACE_TYPE_TOPLEVEL,
    SHELL_SURFACE_TYPE_TRANSIENT,
    SHELL_SURFACE_TYPE_FULLSCREEN,
    SHELL_SURFACE_TYPE_POPUP,
    SHELL_SURFACE_TYPE_MAXIMIZED,
    SHELL_SURFACE_TYPE_MINIMIZED,
} shell_surface_type_t;

struct shell_surface
{
    desktop_shell_t         *shell;

    shell_client_t          *shell_client;

    struct wl_client        *client;
    struct wl_resource      *resource;

    /* Hierarchy */
    pepper_surface_t        *parent;
    pepper_list_t            child_list;   /* children surfaces of this */
    pepper_list_t            parent_link;

    /* Contents */
    pepper_surface_t        *surface;
    pepper_view_t           *view;

    char                    *title, *class_;

    struct
    {
        double x, y;
        int32_t w,h;
    } geometry, next_geometry;

    pepper_bool_t            has_next_geometry;

    const void              *old_pointer_grab;
    void                    *old_pointer_grab_data;

    struct
    {
        double          dx, dy;     /* difference between pointer position and view position */
    } move;

    struct
    {
        double          px, py;     /* pointer x, y */
        double          vx, vy;     /* view    x, y */
        int32_t         vw, vh;     /* view    w, h */
        uint32_t        edges;
        pepper_bool_t   resizing;
    } resize;

    int32_t         last_width, last_height;

    /* Data structures per surface type */
    shell_surface_type_t     type;          /* Current surface type */
    shell_surface_type_t     next_type;     /* Requested surface type */

    struct
    {
        double          x, y;
        uint32_t        flags;
        pepper_seat_t  *seat;
        uint32_t        serial;
        pepper_bool_t   button_up;
    } popup;

    struct
    {
        double               x, y;
        uint32_t             flags;
    } transient;

    struct
    {
        pepper_output_t     *output;
    } maximized;

    struct
    {
        uint32_t             method;
        uint32_t             framerate;
        pepper_output_t     *output;
    } fullscreen;

    struct
    {
        double               x, y;
        int32_t              w, h;
        pepper_output_mode_t mode;
    } saved;

    /* map */
    void (*shell_surface_map)(shell_surface_t *shsurf);

    /* (*map) was called */
    pepper_bool_t            mapped;

    /* configure */
    void (*send_configure)(shell_surface_t *shsurf, int32_t width, int32_t height);

    pepper_bool_t           ack_configure;

    /* Listeners */
    pepper_event_listener_t *surface_destroy_listener;
    pepper_event_listener_t *surface_commit_listener;

    pepper_list_t            link;
};

shell_client_t *
shell_client_create(desktop_shell_t *shell, struct wl_client *client,
             const struct wl_interface *interface, const void *implementation,
             uint32_t version, uint32_t id);

shell_surface_t *
shell_surface_create(shell_client_t *shell, pepper_surface_t *surface, struct wl_client *client,
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
shell_surface_set_parent(shell_surface_t *shsurf, pepper_surface_t *parent);

/* */
shell_surface_t *
get_shsurf_from_surface(pepper_surface_t *surface, desktop_shell_t *shell);

void
set_shsurf_to_surface(pepper_surface_t *surface, shell_surface_t *shsurf);

pepper_bool_t
shell_surface_set_title(shell_surface_t *shsurf, const char* title);

pepper_bool_t
shell_surface_set_class(shell_surface_t *shsurf, const char* class_);

void
shell_surface_set_toplevel(shell_surface_t *shsurf);

void
shell_surface_set_popup(shell_surface_t *shsurf, pepper_seat_t *seat, pepper_surface_t *parent,
                        double x, double y, uint32_t flags, uint32_t serial);

void
shell_surface_set_transient(shell_surface_t *shsurf, pepper_surface_t *parent,
                            double x, double y, uint32_t flags);

void
shell_surface_ack_configure(shell_surface_t *shsurf, uint32_t serial);

void
shell_surface_set_geometry(shell_surface_t *shsurf, double x, double y, int32_t w, int32_t h);

void
shell_surface_set_maximized(shell_surface_t *shsurf, pepper_output_t *output);

void
shell_surface_unset_maximized(shell_surface_t *shsurf);

void
shell_surface_set_fullscreen(shell_surface_t *shsurf, pepper_output_t *output,
                             uint32_t method, uint32_t framerate);

void
shell_surface_unset_fullscreen(shell_surface_t *shsurf);

void
shell_surface_set_minimized(shell_surface_t *shsurf);

void
shell_get_output_workarea(desktop_shell_t *shell, pepper_output_t *output,
                          pixman_rectangle32_t  *area);


pepper_bool_t
init_wl_shell(desktop_shell_t *shell);

void
fini_wl_shell(desktop_shell_t *shell);

pepper_bool_t
init_xdg_shell(desktop_shell_t *shell);

void
fini_xdg_shell(desktop_shell_t *shell);

void
shell_surface_move(shell_surface_t *shsurf, pepper_seat_t *seat, uint32_t serial);

void
shell_surface_resize(shell_surface_t *shsurf, pepper_seat_t *seat, uint32_t serial, uint32_t edges);
