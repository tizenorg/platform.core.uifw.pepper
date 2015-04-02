#include "pepper-wayland.h"
#include <wayland-client.h>
#include <common.h>

typedef struct wayland_output       wayland_output_t;
typedef struct wayland_seat         wayland_seat_t;

struct pepper_wayland
{
    pepper_compositor_t    *pepper;

    char                   *socket_name;
    struct wl_display      *display;
    int                     fd;

    struct wl_event_source *event_source;

    struct wl_registry     *registry;
    struct wl_compositor   *compositor;
    struct wl_shell        *shell;
    struct wl_list          seat_list;

    struct wl_signal        destroy_signal;

};

struct wayland_output
{
    pepper_wayland_t           *conn;

    struct wl_signal            destroy_signal;
    struct wl_signal            mode_change_signal;

    struct wl_listener          conn_destroy_listener;

    int32_t                     w, h;
    int32_t                     subpixel;

    struct wl_surface          *surface;
    struct wl_shell_surface    *shell_surface;
};

struct wayland_seat
{
    pepper_seat_t              *base;

    uint32_t                    id;
    uint32_t                    caps;
    char                       *name;

    struct wl_seat             *seat;
    struct wl_pointer          *pointer;
    struct wl_keyboard         *keyboard;
    struct wl_touch            *touch;

    wl_fixed_t                  pointer_x_last;
    wl_fixed_t                  pointer_y_last;
    wl_fixed_t                  touch_x_last;   /* FIXME */
    wl_fixed_t                  touch_y_last;   /* FIXME */

    struct wl_list              link;
    struct wl_signal            capability_signal;
    struct wl_signal            name_signal;
};

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version);
