#include <config.h>
#include "pepper-desktop-shell.h"
#include <wayland-server.h>

/* Ping timeout value in ms. */
#define DESKTOP_SHELL_PING_TIMEOUT  200

typedef struct desktop_shell    desktop_shell_t;
typedef struct shell_client     shell_client_t;
typedef struct shell_surface    shell_surface_t;
typedef struct shell_seat       shell_seat_t;

typedef struct shell_pointer_grab              shell_pointer_grab_t;
typedef struct shell_pointer_grab_interface    shell_pointer_grab_interface_t;

struct shell_pointer_grab
{
    shell_seat_t                        *shseat;
    pepper_pointer_t                    *pointer;
    shell_pointer_grab_interface_t      *interface;
    void                                *userdata;
};

struct shell_pointer_grab_interface
{
    void (*motion)(shell_pointer_grab_t *grab,
                   int32_t x, int32_t y, uint32_t time, void *data);
    void (*button)(shell_pointer_grab_t *grab,
                   uint32_t button, uint32_t state, uint32_t time, void *data);
    void (*axis)(shell_pointer_grab_t *grab,
                 uint32_t time, enum wl_pointer_axis axis, wl_fixed_t amount, void *data);
};

struct shell_seat
{
    desktop_shell_t             *shell;
    pepper_seat_t               *seat;
    pepper_list_t                link;

    /* grab */
    shell_pointer_grab_t         pointer_grab;
    shell_pointer_grab_t         default_pointer_grab;
    /* TODO: keyboard, touch*/

    /* Seat's logical device add/remove */
    pepper_event_listener_t     *pointer_add_listener;
    pepper_event_listener_t     *pointer_remove_listener;

    pepper_event_listener_t     *keyboard_add_listener;
    pepper_event_listener_t     *keyboard_remove_listener;

    pepper_event_listener_t     *touch_add_listener;
    pepper_event_listener_t     *touch_remove_listener;

    /* Input event listeners */
    pepper_event_listener_t     *pointer_motion_listener;
    pepper_event_listener_t     *pointer_button_listener;
    pepper_event_listener_t     *pointer_axis_listener;

    pepper_event_listener_t     *keyboard_key_listener;
    pepper_event_listener_t     *keyboard_modifiers_listener;

    pepper_event_listener_t     *touch_down_listener;
    pepper_event_listener_t     *touch_up_listener;
    pepper_event_listener_t     *touch_motion_listener;
    pepper_event_listener_t     *touch_frame_listener;
    pepper_event_listener_t     *touch_cancel_listener;
};

struct desktop_shell
{
    pepper_compositor_t         *compositor;

    struct wl_list               shell_client_list;
    struct wl_list               shell_surface_list;

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

    struct wl_list           link;
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
    struct wl_list           child_list;   /* children surfaces of this */
    struct wl_list           parent_link;

    /* Contents */
    pepper_surface_t        *surface;
    pepper_view_t           *view;

    char                    *title, *class_;

    /* Data structures per surface type */
    shell_surface_type_t     type;          /* Current surface type */
    shell_surface_type_t     next_type;     /* Requested surface type */

    struct
    {
        int32_t              x, y;
        uint32_t             flags;
        pepper_seat_t       *seat;
    } popup;

    struct
    {
        int32_t              x, y;
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
        pepper_surface_t    *background_surface;
        pepper_view_t       *background_view;
        pepper_output_t     *output;
    } fullscreen;

    struct
    {
        int32_t              x, y, w, h;
        uint32_t             framerate;
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

    struct wl_list          link;       /* link */
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
                        int32_t x, int32_t y, uint32_t flags);

void
shell_surface_set_transient(shell_surface_t *shsurf, pepper_surface_t *parent,
                            int32_t x, int32_t y, uint32_t flags);

void
shell_surface_ack_configure(shell_surface_t *shsurf, uint32_t serial);

void
shell_surface_set_geometry(shell_surface_t *shsurf, int32_t x, int32_t y, int32_t w, int32_t h);

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
shell_seat_pointer_start_grab(shell_seat_t *shseat, shell_pointer_grab_interface_t *grab, void *userdata);

void
shell_seat_pointer_end_grab(shell_seat_t *shseat);
