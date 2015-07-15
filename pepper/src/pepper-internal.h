#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include "common.h"
#include "pepper.h"
#include <wayland-util.h>
#include <pixman.h>
#include <pepper-output-backend.h>

typedef struct pepper_compositor    pepper_compositor_t;
typedef struct pepper_output        pepper_output_t;
typedef struct pepper_surface       pepper_surface_t;
typedef struct pepper_buffer        pepper_buffer_t;
typedef struct pepper_view          pepper_view_t;
typedef struct pepper_seat          pepper_seat_t;
typedef struct pepper_pointer       pepper_pointer_t;
typedef struct pepper_keyboard      pepper_keyboard_t;
typedef struct pepper_touch         pepper_touch_t;
typedef struct pepper_plane         pepper_plane_t;

#define CHECK_NON_NULL(ptr)                                                     \
    do {                                                                        \
        if ((ptr) == NULL) {                                                    \
            PEPPER_ERROR("NULL check failed.\n");                               \
        }                                                                       \
    } while (0)

#define CHECK_MAGIC(obj, val)                                                   \
    do {                                                                        \
        if (((obj)->magic) != val)                                              \
        {                                                                       \
            PEPPER_ERROR("magic check failed : %p is not an %s\n", obj, #val);  \
        }                                                                       \
    } while (0)

#define CHECK_MAGIC_AND_NON_NULL(obj, val)                                      \
    do {                                                                        \
        CHECK_NON_NULL(obj);                                                    \
        CHECK_MAGIC(obj, val);                                                  \
    } while (0)

#define CHECK_MAGIC_IF_NON_NULL(obj, val)                                       \
    do {                                                                        \
        if (obj)                                                                \
            CHECK_MAGIC(obj, val);                                              \
    } while (0)

#define PEPPER_MAX_OUTPUT_COUNT 32

typedef struct pepper_region        pepper_region_t;
typedef struct pepper_surface_state pepper_surface_state_t;
typedef struct pepper_plane_entry   pepper_plane_entry_t;
typedef struct pepper_data_source   pepper_data_source_t;
typedef struct pepper_data_device   pepper_data_device_t;
typedef struct pepper_data_offer    pepper_data_offer_t;

enum pepper_magic
{
    PEPPER_COMPOSITOR   = 0x00000001,
    PEPPER_OUTPUT       = 0x00000002,
    PEPPER_SURFACE      = 0x00000003,
    PEPPER_BUFFER       = 0x00000004,
    PEPPER_VIEW         = 0x00000005,
    PEPPER_SEAT         = 0x00000006,
    PEPPER_POINTER      = 0x00000007,
    PEPPER_KEYBOARD     = 0x00000008,
    PEPPER_TOUCH        = 0x00000009,
    PEPPER_PLANE        = 0x0000000a,
};

struct pepper_object
{
    uint32_t            magic;
    struct wl_signal    destroy_signal;
    pepper_map_t       *user_data_map;
};

pepper_object_t *
pepper_object_alloc(size_t size, uint32_t magic);

pepper_bool_t
pepper_object_init(pepper_object_t *object, uint32_t magic);

void
pepper_object_fini(pepper_object_t *object);

/* compositor */
struct pepper_compositor
{
    pepper_object_t     base;
    char               *socket_name;
    struct wl_display  *display;

    struct wl_list      surfaces;
    struct wl_list      regions;
    struct wl_list      seat_list;
    pepper_list_t       output_list;
    uint32_t            output_id_allocator;
    struct wl_list      event_hook_chain;
    pepper_list_t       view_list;
};

struct pepper_output
{
    pepper_object_t             base;
    pepper_compositor_t        *compositor;
    uint32_t                    id;

    struct wl_global           *global;
    struct wl_list              resources;
    pepper_list_t               link;

    pepper_output_geometry_t    geometry;
    int32_t                     scale;

    int                         mode_count;
    pepper_output_mode_t       *modes;
    pepper_output_mode_t       *current_mode;

    /* Backend-specific variables. */
    pepper_output_backend_t    *backend;
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
    pepper_buffer_t    *buffer;
    int32_t             x;
    int32_t             y;
    pepper_bool_t       newly_attached;

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
    pepper_object_t         base;
    pepper_compositor_t    *compositor;
    struct wl_resource     *resource;

    struct {
        pepper_buffer_t    *buffer;
        int32_t             x, y;
        int32_t             transform;
        int32_t             scale;
    } buffer;

    /* Surface size in surface local coordinate space.
     * Buffer is transformed and scaled into surface local coordinate space. */
    int32_t                 w, h;

    pixman_region32_t       damage_region;
    pixman_region32_t       opaque_region;
    pixman_region32_t       input_region;

    struct wl_list          frame_callbacks;

    /* Surface states. wl_surface.commit will apply the pending state into current. */
    pepper_surface_state_t  pending;

    char                   *role;
    pepper_map_t           *user_data_map;
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

struct pepper_region
{
    pepper_object_t         base;
    pepper_compositor_t    *compositor;
    struct wl_resource     *resource;
    pixman_region32_t       pixman_region;
};

pepper_region_t *
pepper_region_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id);

void
pepper_region_destroy(pepper_region_t *region);

/* Input */
struct pepper_seat
{
    pepper_object_t             base;
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

    uint32_t                    modifier;

    /* Backend-specific variables. */
    pepper_seat_backend_t      *backend;
    void                       *data;
};

struct pepper_pointer
{
    pepper_object_t             base;
    pepper_seat_t              *seat;
    struct wl_list              resources;
};

struct pepper_keyboard
{
    pepper_object_t             base;
    pepper_seat_t              *seat;
    struct wl_list              resources;
};

struct pepper_touch
{
    pepper_object_t             base;
    pepper_seat_t              *seat;
    struct wl_list              resources;
};

void
pepper_seat_update_modifier(pepper_seat_t *seat, pepper_input_event_t *event);

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
    pepper_render_item_t    base;

    pepper_plane_t         *plane;
    struct wl_listener      plane_destroy_listener;
    pepper_bool_t           need_damage;

    pepper_list_t           link;
};

struct pepper_view
{
    pepper_object_t         base;
    pepper_compositor_t    *compositor;
    pepper_list_t           compositor_link;

    /* Hierarchy. */
    pepper_view_t          *parent;
    pepper_list_t           parent_link;
    pepper_list_t           children_list;

    /* Geometry. */
    pepper_bool_t           geometry_dirty;
    double                  x, y;
    int                     w, h;
    pepper_mat4_t           transform;
    pepper_mat4_t           global_transform;

    pixman_region32_t       bounding_region;
    pixman_region32_t       opaque_region;

    /* Visibility. */
    pepper_bool_t           visibility;
    pepper_bool_t           mapped;

    /* Content. */
    pepper_surface_t       *surface;
    pepper_list_t           surface_link;
    struct wl_listener      surface_destroy_listener;

    /* Output info. */
    uint32_t                output_overlap;
    pepper_plane_entry_t    plane_entries[PEPPER_MAX_OUTPUT_COUNT];

    /* Temporary resource. */
    pepper_list_t           link;
};

void
pepper_view_assign_plane(pepper_object_t *view, pepper_object_t *output, pepper_object_t *plane);

void
pepper_view_damage_below(pepper_view_t *view);

void
pepper_view_update_geometry(pepper_view_t *view);

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
pepper_plane_destroy(pepper_object_t *plane);

pixman_region32_t *
pepper_plane_get_damage_region(pepper_object_t *plane);

pixman_region32_t *
pepper_plane_get_clip_region(pepper_object_t *plane);

const pepper_list_t *
pepper_plane_get_render_node_list(pepper_object_t *plane);

void
pepper_plane_subtract_damage_region(pepper_object_t *plane, pixman_region32_t *region);

void
pepper_plane_add_damage_region(pepper_plane_t *plane, pixman_region32_t *region);

void
pepper_plane_accumulate_damage(pepper_plane_t *plane, pixman_region32_t *clip);

void
pepper_plane_update(pepper_plane_t *plane, const pepper_list_t *view_list);

/* Event hook */
struct pepper_event_hook
{
    pepper_event_handler_t    handler;
    void                     *data;
    struct wl_list            link;

    /* TODO:
     * void *owner, *priority;
     * or something elses
     */
};

pepper_bool_t
pepper_compositor_event_handler(pepper_object_t         *seat,
                                pepper_input_event_t    *event,
                                void                    *data);

void
pepper_surface_flush_damage(pepper_surface_t *surface);

#endif /* PEPPER_INTERNAL_H */
