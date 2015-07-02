#ifndef PEPPER_H
#define PEPPER_H

#include <pepper-utils.h>

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_object            pepper_object_t;

typedef struct pepper_output_geometry   pepper_output_geometry_t;
typedef struct pepper_output_mode       pepper_output_mode_t;
typedef struct pepper_output_interface  pepper_output_interface_t;

typedef struct pepper_seat_interface    pepper_seat_interface_t;

typedef struct pepper_input_event       pepper_input_event_t;
typedef struct pepper_event_hook        pepper_event_hook_t;

typedef struct pepper_view_state        pepper_view_state_t;

struct pepper_view_state
{
    pepper_object_t            *view;
    const pepper_mat4_t        *transform;
    const pixman_region32_t    *bounding;
    const pixman_region32_t    *opaque;
    const pixman_region32_t    *visible;
};

struct pepper_output_geometry
{
    int32_t     x;
    int32_t     y;
    int32_t     w;
    int32_t     h;
    int32_t     subpixel;
    const char  *maker;
    const char  *model;
    int32_t     transform;
};

struct pepper_output_mode
{
    uint32_t    flags;
    int32_t     w, h;
    int32_t     refresh;
};

struct pepper_output_interface
{
    void            (*destroy)(void *output);

    void            (*add_destroy_listener)(void *output, struct wl_listener *listener);
    void            (*add_mode_change_listener)(void *output, struct wl_listener *listener);

    int32_t         (*get_subpixel_order)(void *output);
    const char *    (*get_maker_name)(void *output);
    const char *    (*get_model_name)(void *output);

    int             (*get_mode_count)(void *output);
    void            (*get_mode)(void *output, int index, pepper_output_mode_t *mode);
    pepper_bool_t   (*set_mode)(void *output, const pepper_output_mode_t *mode);

    void            (*repaint)(void *output,
                               const pepper_list_t *view_list, const pixman_region32_t *damage);
    void            (*attach_surface)(void *output, pepper_object_t *surface, int *w, int *h);

    void            (*add_frame_listener)(void *output, struct wl_listener *listener);
};

/* Generic object functions. */
PEPPER_API void
pepper_object_set_user_data(pepper_object_t *object, const void *key, void *data,
                            pepper_free_func_t free_func);

PEPPER_API void *
pepper_object_get_user_data(pepper_object_t *object, const void *key);

PEPPER_API void
pepper_object_add_destroy_listener(pepper_object_t *object, struct wl_listener *listener);

/* Compositor functions. */
PEPPER_API pepper_object_t *
pepper_compositor_create(const char *socket_name);

PEPPER_API void
pepper_compositor_destroy(pepper_object_t *compositor);

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_object_t *compositor);

PEPPER_API pepper_object_t *
pepper_compositor_add_output(pepper_object_t *compositor,
                             const pepper_output_interface_t *interface,
                             void *data);

PEPPER_API pepper_object_t *
pepper_compositor_add_seat(pepper_object_t *compositor,
                           const pepper_seat_interface_t *interface,
                           void *data);

PEPPER_API pepper_object_t *
pepper_output_get_compositor(pepper_object_t *output);

PEPPER_API void
pepper_output_destroy(pepper_object_t *output);

PEPPER_API void
pepper_output_move(pepper_object_t *output, int32_t x, int32_t y);

PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_object_t *output);

PEPPER_API unsigned int
pepper_output_get_scale(pepper_object_t *output);

PEPPER_API int
pepper_output_get_mode_count(pepper_object_t *output);

PEPPER_API const pepper_output_mode_t *
pepper_output_get_mode(pepper_object_t *output, int index);

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_object_t *output, const pepper_output_mode_t *mode);

PEPPER_API void
pepper_output_add_damage(pepper_object_t *output,
                         const pixman_region32_t *region, int x, int y);

PEPPER_API void
pepper_output_add_damage_rect(pepper_object_t *output, int x, int y, unsigned int w, unsigned int h);

PEPPER_API void
pepper_output_add_damage_whole(pepper_object_t *output);

/* Input. */
struct pepper_seat_interface
{
    void            (*destroy)(void *data);
    void            (*add_capabilities_listener)(void *data, struct wl_listener *listener);
    void            (*add_name_listener)(void *data, struct wl_listener *listener);

    uint32_t        (*get_capabilities)(void *data);
    const char *    (*get_name)(void *data);
};

enum pepper_input_event_type
{
    PEPPER_INPUT_EVENT_POINTER_BUTTON,
    PEPPER_INPUT_EVENT_POINTER_MOTION,
    PEPPER_INPUT_EVENT_POINTER_AXIS,
    PEPPER_INPUT_EVENT_KEYBOARD_KEY,
    PEPPER_INPUT_EVENT_TOUCH_DOWN,
    PEPPER_INPUT_EVENT_TOUCH_UP,
    PEPPER_INPUT_EVENT_TOUCH_MOTION,
    PEPPER_INPUT_EVENT_TOUCH_FRAME,
    PEPPER_INPUT_EVENT_TOUCH_CANCEL,
};

enum pepper_input_event_state
{
    PEPPER_INPUT_EVENT_STATE_RELEASED,
    PEPPER_INPUT_EVENT_STATE_PRESSED,
};

enum pepper_input_event_axis
{
    PEPPER_INPUT_EVENT_AXIS_VERTICAL,
    PEPPER_INPUT_EVENT_AXIS_HORIZONTAL,
};

struct pepper_input_event
{
    uint32_t        type;
    uint32_t        time;
    uint32_t        serial;
    uint32_t        index;  /* button, key, touch id or axis */
    uint32_t        state;
    wl_fixed_t      value;
    wl_fixed_t      x;
    wl_fixed_t      y;
};

PEPPER_API pepper_bool_t
pepper_seat_handle_event(pepper_object_t *seat, pepper_input_event_t *event);

/* Event hook */
typedef pepper_bool_t (*pepper_event_handler_t)(pepper_object_t *, pepper_input_event_t *, void *);

PEPPER_API pepper_event_hook_t *
pepper_compositor_add_event_hook(pepper_object_t        *compositor,
                                 pepper_event_handler_t      handler,
                                 void                       *data);

PEPPER_API void
pepper_event_hook_destroy(pepper_event_hook_t     *hook);

/* Surface. */
PEPPER_API const char *
pepper_surface_get_role(pepper_object_t *surface);

PEPPER_API pepper_bool_t
pepper_surface_set_role(pepper_object_t *surface, const char *role);

PEPPER_API pepper_object_t *
pepper_surface_get_buffer(pepper_object_t *surface);

PEPPER_API void
pepper_surface_get_buffer_offset(pepper_object_t *surface, int32_t *x, int32_t *y);

PEPPER_API int32_t
pepper_surface_get_buffer_scale(pepper_object_t *surface);

PEPPER_API int32_t
pepper_surface_get_buffer_transform(pepper_object_t *surface);

PEPPER_API const pixman_region32_t *
pepper_surface_get_damage_region(pepper_object_t *surface);

PEPPER_API const pixman_region32_t *
pepper_surface_get_opaque_region(pepper_object_t *surface);

PEPPER_API const pixman_region32_t *
pepper_surface_get_input_region(pepper_object_t *surface);

/* Buffer. */
PEPPER_API void
pepper_buffer_reference(pepper_object_t *buffer);

PEPPER_API void
pepper_buffer_unreference(pepper_object_t *buffer);

PEPPER_API struct wl_resource *
pepper_buffer_get_resource(pepper_object_t *buffer);

/* View. */
PEPPER_API pepper_object_t *
pepper_compositor_add_surface_view(pepper_object_t *compositor, pepper_object_t *surface);

PEPPER_API void
pepper_view_destroy(pepper_object_t *view);

PEPPER_API pepper_object_t *
pepper_view_get_compositor(pepper_object_t *view);

PEPPER_API pepper_object_t *
pepper_view_get_surface(pepper_object_t *view);

PEPPER_API void
pepper_view_set_parent(pepper_object_t *view, pepper_object_t *parent);

PEPPER_API pepper_object_t *
pepper_view_get_parent(pepper_object_t *view);

PEPPER_API pepper_bool_t
pepper_view_stack_above(pepper_object_t *view, pepper_object_t *below, pepper_bool_t subtree);

PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_object_t *view, pepper_object_t *above, pepper_bool_t subtree);

PEPPER_API void
pepper_view_stack_top(pepper_object_t *view, pepper_bool_t subtree);

PEPPER_API void
pepper_view_stack_bottom(pepper_object_t *view, pepper_bool_t subtree);

PEPPER_API pepper_object_t *
pepper_view_get_above(pepper_object_t *view);

PEPPER_API pepper_object_t *
pepper_view_get_below(pepper_object_t *view);

PEPPER_API const pepper_list_t *
pepper_view_get_children_list(pepper_object_t *view);

PEPPER_API void
pepper_view_resize(pepper_object_t *view, int w, int h);

PEPPER_API void
pepper_view_get_size(pepper_object_t *view, int *w, int *h);

PEPPER_API void
pepper_view_set_position(pepper_object_t *view, double x, double y);

PEPPER_API void
pepper_view_get_position(pepper_object_t *view, double *x, double *y);

PEPPER_API void
pepper_view_set_transform(pepper_object_t *view, const pepper_mat4_t *matrix);

PEPPER_API const pepper_mat4_t *
pepper_view_get_transform(pepper_object_t *view);

PEPPER_API void
pepper_view_map(pepper_object_t *view);

PEPPER_API void
pepper_view_unmap(pepper_object_t *view);

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_object_t *view);

PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_object_t *view);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
