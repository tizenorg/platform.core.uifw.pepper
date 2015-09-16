#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include "pepper.h"
#include <wayland-util.h>
#include <pixman.h>
#include "pepper-output-backend.h"
#include "pepper-input-backend.h"

#define PEPPER_OBJECT_BUCKET_BITS   5
#define PEPPER_MAX_OUTPUT_COUNT     32

typedef struct pepper_region        pepper_region_t;
typedef struct pepper_surface_state pepper_surface_state_t;
typedef struct pepper_plane_entry   pepper_plane_entry_t;
typedef struct pepper_data_source   pepper_data_source_t;
typedef struct pepper_data_device   pepper_data_device_t;
typedef struct pepper_data_offer    pepper_data_offer_t;
typedef struct pepper_input         pepper_input_t;

struct pepper_object
{
    pepper_object_type_t    type;
    pepper_map_t            user_data_map;
    pepper_map_entry_t     *buckets[1 << PEPPER_OBJECT_BUCKET_BITS];
    void                    (*handle_event)(pepper_object_t *object, uint32_t id, void *info);
    pepper_list_t           event_listener_list;
};

pepper_object_t *
pepper_object_alloc(pepper_object_type_t type, size_t size);

void
pepper_object_init(pepper_object_t *object, pepper_object_type_t type);

void
pepper_object_fini(pepper_object_t *object);

struct pepper_event_listener
{
    pepper_object_t             *object;
    uint32_t                    id;
    int                         priority;
    pepper_event_callback_t     callback;
    void                       *data;

    pepper_list_t               link;
};

/* compositor */
struct pepper_compositor
{
    pepper_object_t     base;
    char               *socket_name;
    struct wl_display  *display;
    struct wl_global   *global;

    pepper_list_t       surface_list;
    pepper_list_t       region_list;
    pepper_list_t       seat_list;
    pepper_list_t       output_list;
    pepper_list_t       view_list;
    pepper_list_t       input_device_list;

    uint32_t            output_id_allocator;
    pepper_bool_t       update_scheduled;

    clockid_t           clock_id;
    pepper_bool_t       clock_used;
};

void
pepper_compositor_schedule_repaint(pepper_compositor_t *compositor);

struct pepper_output
{
    pepper_object_t             base;
    pepper_compositor_t        *compositor;
    uint32_t                    id;
    char                       *name;

    struct wl_global           *global;
    struct wl_list              resource_list;
    pepper_list_t               link;

    pepper_output_geometry_t    geometry;
    int32_t                     scale;

    pepper_output_mode_t        current_mode;

    /* Backend-specific variables. */
    pepper_output_backend_t    *backend;
    void                       *data;

    /* Frame state flags. */
    struct {
        pepper_bool_t           scheduled;
        pepper_bool_t           pending;
        struct timespec         time;
    } frame;

    pepper_list_t               plane_list;
    pepper_list_t               view_list;
};

void
pepper_output_schedule_repaint(pepper_output_t *output);

struct pepper_buffer
{
    pepper_object_t         base;
    struct wl_resource     *resource;

    int                     ref_count;
    struct wl_listener      resource_destroy_listener;

    /* the buffer size is unknown until it is actually attached to a renderer. */
    int32_t                 w, h;
};

pepper_buffer_t *
pepper_buffer_from_resource(struct wl_resource *resource);

struct pepper_surface_state
{
    pepper_buffer_t            *buffer;
    int32_t                     x;
    int32_t                     y;
    pepper_bool_t               newly_attached;

    int32_t                     transform;
    int32_t                     scale;

    pixman_region32_t           damage_region;
    pixman_region32_t           opaque_region;
    pixman_region32_t           input_region;

    struct wl_list              frame_callback_list;
    pepper_event_listener_t    *buffer_destroy_listener;
};

struct pepper_surface
{
    pepper_object_t         base;
    pepper_compositor_t    *compositor;
    struct wl_resource     *resource;
    pepper_list_t           link;

    struct {
        pepper_buffer_t         *buffer;
        pepper_event_listener_t *destroy_listener;
        int32_t                  x, y;
        int32_t                  transform;
        int32_t                  scale;
    } buffer;

    /* Surface size in surface local coordinate space.
     * Buffer is transformed and scaled into surface local coordinate space. */
    int32_t                 w, h;

    pixman_region32_t       damage_region;
    pixman_region32_t       opaque_region;
    pixman_region32_t       input_region;

    struct wl_list          frame_callback_list;

    /* Surface states. wl_surface.commit will apply the pending state into current. */
    pepper_surface_state_t  pending;

    char                   *role;
    pepper_list_t           view_list;
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

void
pepper_surface_send_frame_callback_done(pepper_surface_t *surface, uint32_t time);

struct pepper_region
{
    pepper_object_t         base;
    pepper_compositor_t    *compositor;
    struct wl_resource     *resource;
    pepper_list_t           link;

    pixman_region32_t       pixman_region;
};

pepper_region_t *
pepper_region_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id);

void
pepper_region_destroy(pepper_region_t *region);

void
pepper_transform_pixman_region(pixman_region32_t *region, const pepper_mat4_t *matrix);

/* Input */
struct pepper_input
{
    pepper_seat_t      *seat;
    struct wl_list      resource_list;
    pepper_view_t      *focus;
    struct wl_listener  focus_destroy_listener;
    struct wl_list      focus_resource_list;
};

void
pepper_input_init(pepper_input_t *input, pepper_seat_t *seat);

void
pepper_input_fini(pepper_input_t *input);

void
pepper_input_bind_resource(pepper_input_t *input,
                           struct wl_client *client, int version, uint32_t id,
                           const struct wl_interface *interface, const void *impl, void *data);

void
pepper_input_set_focus(pepper_input_t *input, pepper_view_t *view);

struct pepper_pointer
{
    pepper_object_t                 base;
    pepper_input_t                  input;

    const pepper_pointer_grab_t    *grab;
    void                           *data;

    double                          x, y;
    double                          vx, vy;
};

pepper_pointer_t *
pepper_pointer_create(pepper_seat_t *seat);

void
pepper_pointer_destroy(pepper_pointer_t *pointer);

void
pepper_pointer_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id);

struct pepper_keyboard
{
    pepper_object_t                 base;
    pepper_input_t                  input;

    const pepper_keyboard_grab_t   *grab;
    void                           *data;
};

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat);

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard);

void
pepper_keyboard_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id);

struct pepper_touch
{
    pepper_object_t                 base;
    pepper_input_t                  input;

    const pepper_touch_grab_t      *grab;
    void                           *data;
};

pepper_touch_t *
pepper_touch_create(pepper_seat_t *seat);

void
pepper_touch_destroy(pepper_touch_t *touch);

void
pepper_touch_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id);

struct pepper_seat
{
    pepper_object_t             base;
    pepper_compositor_t        *compositor;
    pepper_list_t               link;
    char                       *name;
    struct wl_global           *global;
    struct wl_list              resource_list;

    enum wl_seat_capability     caps;
    uint32_t                    modifier;

    pepper_pointer_t           *pointer;
    pepper_keyboard_t          *keyboard;
    pepper_touch_t             *touch;

    pepper_list_t               input_device_list;
};

struct pepper_input_device
{
    pepper_object_t                         base;
    pepper_compositor_t                    *compositor;
    pepper_list_t                           link;

    uint32_t                                caps;

    void                                   *data;
    const pepper_input_device_backend_t    *backend;
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

struct pepper_plane_entry
{
    pepper_render_item_t        base;

    pepper_plane_t             *plane;
    pepper_bool_t               need_damage;

    pepper_list_t               link;
};

enum
{
    PEPPER_VIEW_GEOMETRY_DIRTY      = 0x00000001,
    PEPPER_VIEW_ACTIVE_DIRTY        = 0x00000002,
    PEPPER_VIEW_Z_ORDER_DIRTY       = 0x00000004,
    PEPPER_VIEW_CONTENT_DIRTY       = 0x00000008,
};

struct pepper_view
{
    pepper_object_t             base;
    pepper_compositor_t        *compositor;
    pepper_list_t               compositor_link;

    uint32_t                    dirty;

    /* Hierarchy. */
    pepper_view_t              *parent;
    pepper_list_t               parent_link;
    pepper_list_t               children_list;

    /* Geometry. */
    double                      x, y;
    int                         w, h;
    pepper_mat4_t               transform;
    pepper_mat4_t               global_transform;
    pepper_mat4_t               global_transform_inverse;

    pixman_region32_t           bounding_region;
    pixman_region32_t           opaque_region;

    /* Visibility. */
    pepper_bool_t               active;
    pepper_bool_t               prev_visible;
    pepper_bool_t               mapped;

    /* Content. */
    pepper_surface_t           *surface;
    pepper_list_t               surface_link;


    /* Output info. */
    uint32_t                    output_overlap;
    pepper_plane_entry_t        plane_entries[PEPPER_MAX_OUTPUT_COUNT];

    /* Temporary resource. */
    pepper_list_t               link;
};

void
pepper_view_mark_dirty(pepper_view_t *view, uint32_t flag);

void
pepper_view_update(pepper_view_t *view);

void
pepper_view_surface_damage(pepper_view_t *view);

void
pepper_view_get_local_coordinate(pepper_view_t *view,
                                 double global_x, double global_y,
                                 double *local_x, double *local_y);

void
pepper_view_get_global_coordinate(pepper_view_t *view,
                                  double local_x, double local_y,
                                  double *global_x, double *global_y);

struct pepper_plane
{
    pepper_object_t     base;
    pepper_output_t    *output;

    pepper_list_t       entry_list;
    pixman_region32_t   damage_region;
    pixman_region32_t   clip_region;

    pepper_list_t       link;
};

pepper_object_t *
pepper_plane_create(pepper_object_t *output, pepper_object_t *above_plane);

void
pepper_plane_add_damage_region(pepper_plane_t *plane, pixman_region32_t *region);

void
pepper_plane_accumulate_damage(pepper_plane_t *plane, pixman_region32_t *clip);

void
pepper_plane_update(pepper_plane_t *plane, const pepper_list_t *view_list);

void
pepper_surface_flush_damage(pepper_surface_t *surface);

#endif /* PEPPER_INTERNAL_H */
