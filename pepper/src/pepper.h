#ifndef PEPPER_H
#define PEPPER_H

#include <pepper-utils.h>

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_object            pepper_object_t;

typedef struct pepper_compositor        pepper_compositor_t;
typedef struct pepper_output            pepper_output_t;
typedef struct pepper_surface           pepper_surface_t;
typedef struct pepper_buffer            pepper_buffer_t;
typedef struct pepper_view              pepper_view_t;
typedef struct pepper_seat              pepper_seat_t;
typedef struct pepper_pointer           pepper_pointer_t;
typedef struct pepper_keyboard          pepper_keyboard_t;
typedef struct pepper_touch             pepper_touch_t;

typedef struct pepper_output_geometry   pepper_output_geometry_t;
typedef struct pepper_output_mode       pepper_output_mode_t;
typedef struct pepper_input_event       pepper_input_event_t;
typedef struct pepper_event_hook        pepper_event_hook_t;

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

/* Generic object functions. */
PEPPER_API void
pepper_object_set_user_data(pepper_object_t *object, const void *key, void *data,
                            pepper_free_func_t free_func);

PEPPER_API void *
pepper_object_get_user_data(pepper_object_t *object, const void *key);

PEPPER_API void
pepper_object_add_destroy_listener(pepper_object_t *object, struct wl_listener *listener);

/* Compositor functions. */
PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name);

PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor);

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor);

PEPPER_API pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output);

PEPPER_API void
pepper_output_destroy(pepper_output_t *output);

PEPPER_API void
pepper_output_move(pepper_output_t *output, int32_t x, int32_t y);

PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_output_t *output);

PEPPER_API unsigned int
pepper_output_get_scale(pepper_output_t *output);

PEPPER_API int
pepper_output_get_mode_count(pepper_output_t *output);

PEPPER_API const pepper_output_mode_t *
pepper_output_get_mode(pepper_output_t *output, int index);

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_output_t *output, const pepper_output_mode_t *mode);

/* Input. */
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
pepper_seat_handle_event(pepper_seat_t *seat, pepper_input_event_t *event);

/* Event hook */
typedef pepper_bool_t (*pepper_event_handler_t)(pepper_seat_t *, pepper_input_event_t *, void *);

PEPPER_API pepper_event_hook_t *
pepper_compositor_add_event_hook(pepper_compositor_t        *compositor,
                                 pepper_event_handler_t      handler,
                                 void                       *data);

PEPPER_API void
pepper_event_hook_destroy(pepper_event_hook_t *hook);

/* Surface. */
PEPPER_API const char *
pepper_surface_get_role(pepper_surface_t *surface);

PEPPER_API pepper_bool_t
pepper_surface_set_role(pepper_surface_t *surface, const char *role);

PEPPER_API pepper_buffer_t *
pepper_surface_get_buffer(pepper_surface_t *surface);

PEPPER_API void
pepper_surface_get_buffer_offset(pepper_surface_t *surface, int32_t *x, int32_t *y);

PEPPER_API int32_t
pepper_surface_get_buffer_scale(pepper_surface_t *surface);

PEPPER_API int32_t
pepper_surface_get_buffer_transform(pepper_surface_t *surface);

PEPPER_API const pixman_region32_t *
pepper_surface_get_damage_region(pepper_surface_t *surface);

PEPPER_API const pixman_region32_t *
pepper_surface_get_opaque_region(pepper_surface_t *surface);

PEPPER_API const pixman_region32_t *
pepper_surface_get_input_region(pepper_surface_t *surface);

/* Buffer. */
PEPPER_API void
pepper_buffer_reference(pepper_buffer_t *buffer);

PEPPER_API void
pepper_buffer_unreference(pepper_buffer_t *buffer);

PEPPER_API struct wl_resource *
pepper_buffer_get_resource(pepper_buffer_t *buffer);

/* View. */
PEPPER_API pepper_view_t *
pepper_compositor_add_surface_view(pepper_compositor_t *compositor, pepper_surface_t *surface);

PEPPER_API void
pepper_view_destroy(pepper_view_t *view);

PEPPER_API pepper_compositor_t *
pepper_view_get_compositor(pepper_view_t *view);

PEPPER_API pepper_surface_t *
pepper_view_get_surface(pepper_view_t *view);

PEPPER_API void
pepper_view_set_parent(pepper_view_t *view, pepper_view_t *parent);

PEPPER_API pepper_view_t *
pepper_view_get_parent(pepper_view_t *view);

PEPPER_API pepper_bool_t
pepper_view_stack_above(pepper_view_t *view, pepper_view_t *below, pepper_bool_t subtree);

PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_view_t *view, pepper_view_t *above, pepper_bool_t subtree);

PEPPER_API void
pepper_view_stack_top(pepper_view_t *view, pepper_bool_t subtree);

PEPPER_API void
pepper_view_stack_bottom(pepper_view_t *view, pepper_bool_t subtree);

PEPPER_API pepper_view_t *
pepper_view_get_above(pepper_view_t *view);

PEPPER_API pepper_view_t *
pepper_view_get_below(pepper_view_t *view);

PEPPER_API const pepper_list_t *
pepper_view_get_children_list(pepper_view_t *view);

PEPPER_API void
pepper_view_resize(pepper_view_t *view, int w, int h);

PEPPER_API void
pepper_view_get_size(pepper_view_t *view, int *w, int *h);

PEPPER_API void
pepper_view_set_position(pepper_view_t *view, double x, double y);

PEPPER_API void
pepper_view_get_position(pepper_view_t *view, double *x, double *y);

PEPPER_API void
pepper_view_set_transform(pepper_view_t *view, const pepper_mat4_t *matrix);

PEPPER_API const pepper_mat4_t *
pepper_view_get_transform(pepper_view_t *view);

PEPPER_API void
pepper_view_map(pepper_view_t *view);

PEPPER_API void
pepper_view_unmap(pepper_view_t *view);

PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_view_t *view);

PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_view_t *view);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
