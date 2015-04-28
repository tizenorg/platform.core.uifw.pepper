#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include "common.h"
#include "pepper.h"
#include <wayland-util.h>
#include <pixman.h>

typedef struct pepper_region        pepper_region_t;
typedef struct pepper_surface_state pepper_surface_state_t;
typedef struct pepper_buffer        pepper_buffer_t;
typedef struct pepper_data_source   pepper_data_source_t;
typedef struct pepper_data_device   pepper_data_device_t;
typedef struct pepper_data_offer    pepper_data_offer_t;

/* compositor */
struct pepper_compositor
{
    char               *socket_name;
    struct wl_display  *display;
    struct wl_list      surfaces;
    struct wl_list      regions;
    struct wl_list      seat_list;
};

struct pepper_output
{
    pepper_compositor_t        *compositor;

    struct wl_global           *global;
    struct wl_list              resources;

    pepper_output_geometry_t    geometry;
    int32_t                     scale;

    int                         mode_count;
    pepper_output_mode_t       *modes;
    pepper_output_mode_t       *current_mode;

    /* Backend-specific variables. */
    pepper_output_interface_t  *interface;
    void                       *data;

    /* Listeners for backend-side events. */
    struct wl_listener          data_destroy_listener;
    struct wl_listener          mode_change_listener;

    /* Frame state flags. */
    struct {
        pepper_bool_t           scheduled;
        pepper_bool_t           pending;
        struct wl_listener      frame_listener;
    } frame;
};

void
pepper_output_schedule_repaint(pepper_output_t *output);

void
pepper_output_repaint(pepper_output_t *output);

struct pepper_buffer
{
    struct wl_resource     *resource;

    int                     ref_count;
    struct wl_signal        destroy_signal;
    struct wl_listener      resource_destroy_listener;

    /* the buffer size is unknown until it is actually attached to a renderer. */
    int32_t                 w, h;
};

struct pepper_surface_state
{
    pepper_buffer_t    *buffer;
    int32_t             x;
    int32_t             y;
    int32_t             transform;
    int32_t             scale;

    pixman_region32_t   damage_region;
    pixman_region32_t   opaque_region;
    pixman_region32_t   input_region;

    struct wl_list      frame_callbacks;
    struct wl_listener  buffer_destroy_listener;
};

struct pepper_surface
{
    pepper_compositor_t    *compositor;
    struct wl_resource     *resource;

    /* Surface states. wl_surface.commit will apply the pending state into current. */
    pepper_surface_state_t  pending;

    pepper_buffer_t        *buffer;
    int32_t                 offset_x, offset_y;
    int32_t                 transform;
    int32_t                 scale;

    int32_t                 w, h;

    pixman_region32_t       damage_region;
    pixman_region32_t       opaque_region;
    pixman_region32_t       input_region;

    struct wl_list          frame_callbacks;
    struct wl_signal        destroy_signal;

    /* Role. */
    char                   *role;
};

struct pepper_region
{
    pepper_compositor_t    *compositor;
    struct wl_resource     *resource;
    pixman_region32_t       pixman_region;
};

pepper_surface_t *
pepper_surface_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id);

void
pepper_surface_destroy(pepper_surface_t *surface);

void
pepper_surface_commit(pepper_surface_t *surface);

pepper_region_t *
pepper_region_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id);

void
pepper_region_destroy(pepper_region_t *region);

pepper_buffer_t *
pepper_buffer_from_resource(struct wl_resource *resource);

void
pepper_buffer_reference(pepper_buffer_t *buffer);

void
pepper_buffer_unreference(pepper_buffer_t *buffer);

/* Input */
struct pepper_seat
{
    pepper_compositor_t        *compositor;
    pepper_pointer_t           *pointer;
    pepper_keyboard_t          *keyboard;
    pepper_touch_t             *touch;

    struct wl_global           *global;
    struct wl_list              resources;
    struct wl_list              link;

    struct wl_listener          capabilities_listener;
    struct wl_listener          name_listener;

    enum wl_seat_capability     caps;
    const char                 *name;

    /* Backend-specific variables. */
    pepper_seat_interface_t    *interface;
    void                       *data;
};

struct pepper_pointer
{
    pepper_seat_t              *seat;
    struct wl_list              resources;
};

struct pepper_keyboard
{
    pepper_seat_t              *seat;
    struct wl_list              resources;
};

struct pepper_touch
{
    pepper_seat_t              *seat;
    struct wl_list              resources;
};

/* Data device */
struct pepper_data_source
{
    struct wl_resource      *resource;
    struct wl_signal         destroy_signal;
    struct wl_array          mime_types;
};

struct pepper_data_offer
{
    struct wl_resource      *resource;
    pepper_data_source_t    *source;
    struct wl_listener       source_destroy_listener;
};

struct pepper_data_device
{
    struct wl_resource      *resource;
    pepper_seat_t           *seat;
};

pepper_bool_t
pepper_data_device_manager_init(struct wl_display *display);

#endif /* PEPPER_INTERNAL_H */
