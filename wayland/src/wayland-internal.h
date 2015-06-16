#include <config.h>
#include "pepper-wayland.h"
#include <wayland-client.h>
#include <pixman.h>
#include <pepper-render.h>

#if ENABLE_WAYLAND_BACKEND_EGL
#include <wayland-egl.h>
#endif

/* TODO: Error logging. */
#define PEPPER_ERROR(...)
#define PEPPER_ASSERT(exp)

#define NUM_SHM_BUFFERS 2

typedef struct wayland_output       wayland_output_t;
typedef struct wayland_seat         wayland_seat_t;
typedef struct wayland_shm_buffer   wayland_shm_buffer_t;

struct pepper_wayland
{
    pepper_object_t        *pepper;

    char                   *socket_name;
    struct wl_display      *display;
    int                     fd;

    struct wl_event_source *event_source;

    struct wl_registry     *registry;
    struct wl_compositor   *compositor;
    struct wl_shell        *shell;
    struct wl_list          seat_list;

    struct wl_signal        destroy_signal;

    struct wl_shm          *shm;

};

struct wayland_shm_buffer
{
    wayland_output_t   *output;
    struct wl_list      link;

    struct wl_buffer   *buffer;

    void               *pixels;
    int                 stride;
    int                 size;
    int                 w, h;

    pixman_image_t     *image;
    pixman_region32_t   damage;

    void               *data;
};

struct wayland_output
{
    pepper_wayland_t           *conn;
    pepper_object_t            *base;

    struct wl_signal            destroy_signal;
    struct wl_signal            mode_change_signal;
    struct wl_signal            frame_signal;

    struct wl_listener          conn_destroy_listener;

    int32_t                     w, h;
    int32_t                     subpixel;

    struct wl_surface          *surface;
    struct wl_shell_surface    *shell_surface;

    pepper_renderer_t          *renderer;

    void    (*render_pre)(wayland_output_t *output);
    void    (*render_post)(wayland_output_t *output);

    struct {
        /* list containing free buffers. */
        struct wl_list          free_buffers;

        /* list containing attached but not released (from the compositor) buffers. */
        struct wl_list          attached_buffers;

        /* current render target buffer. */
        wayland_shm_buffer_t   *current_buffer;
    } shm;

#if ENABLE_WAYLAND_BACKEND_EGL
    struct {
        struct wl_egl_window   *window;
    } egl;
#endif
};

struct wayland_seat
{
    pepper_object_t            *base;

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
    struct wl_signal            capabilities_signal;
    struct wl_signal            name_signal;
};

void
wayland_handle_global_seat(pepper_wayland_t *conn, struct wl_registry *registry,
                           uint32_t name, uint32_t version);

wayland_shm_buffer_t *
wayland_shm_buffer_create(wayland_output_t *output);

void
wayland_shm_buffer_destroy(wayland_shm_buffer_t *buffer);

char *
string_alloc(int len);

char *
string_copy(const char *str);

void
string_free(char *str);
