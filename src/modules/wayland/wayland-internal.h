#include "pepper-wayland.h"
#include <wayland-client.h>
#include <common.h>

typedef struct wayland_output       wayland_output_t;

struct pepper_wayland
{
    pepper_compositor_t    *compositor;

    char                   *socket_name;
    struct wl_display      *display;
    int                     fd;

    struct wl_event_source *event_source;

    struct wl_registry     *registry;
    struct wl_compositor   *compositor;
    struct wl_seat         *seat;
    struct wl_pointer      *pointer;
    struct wl_keyboard     *keyboard;
    struct wl_touch        *touch;
    struct wl_shell        *shell;

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

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version);
