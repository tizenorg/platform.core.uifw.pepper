#include "pepper-wayland.h"
#include <wayland-client.h>
#include <common.h>

typedef struct wayland_connection   wayland_connection_t;
typedef struct wayland_data         wayland_data_t;
typedef struct wayland_output       wayland_output_t;

struct wayland_connection
{
    wayland_data_t          *data;

    char                    *socket_name;
    struct wl_display       *display;
    int                     fd;

    struct wl_event_source  *event_source;

    struct wl_registry      *registry;
    struct wl_compositor    *compositor;
    struct wl_seat          *seat;
    struct wl_pointer       *pointer;
    struct wl_keyboard      *keyboard;
    struct wl_touch         *touch;
    struct wl_shell         *shell;

    struct wl_list          link;
};

struct wayland_data
{
    pepper_compositor_t *compositor;
    struct wl_list      connections;
};

struct wayland_output
{
    wayland_connection_t    *connection;
    pepper_output_t         *base;

    int32_t                 w, h;
    int32_t                 subpixel;
    int32_t                 scale;

    struct wl_surface       *surface;
    struct wl_shell_surface *shell_surface;
};

wayland_connection_t *
wayland_get_connection(pepper_compositor_t *compositor, const char *socket_name);

void
wayland_handle_global_seat(wayland_connection_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version);
