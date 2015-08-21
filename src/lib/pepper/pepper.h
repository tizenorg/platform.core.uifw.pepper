#ifndef PEPPER_H
#define PEPPER_H

#include <pepper-utils.h>

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_object                    pepper_object_t;

typedef struct pepper_compositor                pepper_compositor_t;
typedef struct pepper_output                    pepper_output_t;
typedef struct pepper_surface                   pepper_surface_t;
typedef struct pepper_buffer                    pepper_buffer_t;
typedef struct pepper_view                      pepper_view_t;
typedef struct pepper_seat                      pepper_seat_t;
typedef struct pepper_pointer                   pepper_pointer_t;
typedef struct pepper_keyboard                  pepper_keyboard_t;
typedef struct pepper_touch                     pepper_touch_t;

typedef struct pepper_output_geometry           pepper_output_geometry_t;
typedef struct pepper_output_mode               pepper_output_mode_t;

typedef struct pepper_input_device              pepper_input_device_t;

typedef struct pepper_pointer_motion_event      pepper_pointer_motion_event_t;
typedef struct pepper_pointer_button_event      pepper_pointer_button_event_t;
typedef struct pepper_pointer_axis_event        pepper_pointer_axis_event_t;
typedef struct pepper_keyboard_key_event        pepper_keyboard_key_event_t;
typedef struct pepper_touch_down_event          pepper_touch_down_event_t;
typedef struct pepper_touch_up_event            pepper_touch_up_event_t;
typedef struct pepper_touch_motion_event        pepper_touch_motion_event_t;

typedef struct pepper_event_listener            pepper_event_listener_t;
typedef void (*pepper_event_callback_t)(pepper_event_listener_t *listener, pepper_object_t *object,
                                        uint32_t id, void *info, void *data);

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

typedef enum pepper_object_type
{
    PEPPER_OBJECT_COMPOSITOR,
    PEPPER_OBJECT_OUTPUT,
    PEPPER_OBJECT_SURFACE,
    PEPPER_OBJECT_BUFFER,
    PEPPER_OBJECT_VIEW,
    PEPPER_OBJECT_SEAT,
    PEPPER_OBJECT_POINTER,
    PEPPER_OBJECT_KEYBOARD,
    PEPPER_OBJECT_TOUCH,
    PEPPER_OBJECT_INPUT_DEVICE,
    PEPPER_OBJECT_PLANE,
} pepper_object_type_t;

enum pepper_common_events
{
    PEPPER_EVENT_ALL,
    PEPPER_EVENT_OBJECT_DESTROY,
    PEPPER_COMMON_EVENT_COUNT,
};

enum pepper_compositor_events
{
    PEPPER_EVENT_COMPOSITOR_OUTPUT_ADD = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_COMPOSITOR_OUTPUT_REMOVE,
    PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
    PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
    PEPPER_EVENT_COMPOSITOR_SURFACE_ADD,
    PEPPER_EVENT_COMPOSITOR_SURFACE_REMOVE,
    PEPPER_EVENT_COMPOSITOR_VIEW_ADD,
    PEPPER_EVENT_COMPOSITOR_VIEW_REMOVE,
    PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
    PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,
};

enum pepper_output_events
{
    PEPPER_EVENT_OUTPUT_MODE_CHANGE = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_OUTPUT_MOVE,
};

enum pepper_surface_events
{
    PEPPER_EVENT_SURFACE_COMMIT = PEPPER_COMMON_EVENT_COUNT,
};

enum pepper_buffer_events
{
    PEPPER_EVENT_BUFFER_RELEASE = PEPPER_COMMON_EVENT_COUNT,
};

enum pepper_view_events
{
    PEPPER_EVENT_VIEW_STACK_CHANGE = PEPPER_COMMON_EVENT_COUNT,
};

enum pepper_seat_events
{
    PEPPER_EVENT_SEAT_POINTER_ADD = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_SEAT_POINTER_REMOVE,
    PEPPER_EVENT_SEAT_KEYBOARD_ADD,
    PEPPER_EVENT_SEAT_KEYBOARD_REMOVE,
    PEPPER_EVENT_SEAT_TOUCH_ADD,
    PEPPER_EVENT_SEAT_TOUCH_REMOVE,

    PEPPER_EVENT_SEAT_POINTER_DEVICE_ADD,
    PEPPER_EVENT_SEAT_POINTER_DEVICE_REMOVE,
    PEPPER_EVENT_SEAT_KEYBOARD_DEVICE_ADD,
    PEPPER_EVENT_SEAT_KEYBOARD_DEVICE_REMOVE,
    PEPPER_EVENT_SEAT_TOUCH_DEVICE_ADD,
    PEPPER_EVENT_SEAT_TOUCH_DEVICE_REMOVE,
};

enum pepper_pointer_events
{
    PEPPER_EVENT_POINTER_MOTION = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_POINTER_BUTTON,
    PEPPER_EVENT_POINTER_AXIS,
};

enum pepper_keyboard_events
{
    PEPPER_EVENT_KEYBOARD_KEY = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_KEYBOARD_MODIFIERS,
};

enum pepper_touch_events
{
    PEPPER_EVENT_TOUCH_DOWN = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_TOUCH_UP,
    PEPPER_EVENT_TOUCH_MOTION,
    PEPPER_EVENT_TOUCH_FRAME,
    PEPPER_EVENT_TOUCH_CANCEL,
};

enum pepper_input_device_events
{
    PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION = PEPPER_COMMON_EVENT_COUNT,
    PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE,
    PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON,
    PEPPER_EVENT_INPUT_DEVICE_POINTER_AXIS,

    PEPPER_EVENT_INPUT_DEVICE_KEYBOARD_KEY,

    PEPPER_EVENT_INPUT_DEVICE_TOUCH_DOWN,
    PEPPER_EVENT_INPUT_DEVICE_TOUCH_UP,
    PEPPER_EVENT_INPUT_DEVICE_TOUCH_MOTION,
    PEPPER_EVENT_INPUT_DEVICE_TOUCH_FRAME,
    PEPPER_EVENT_INPUT_DEVICE_TOUCH_CANCEL,
};

struct pepper_pointer_motion_event
{
    uint32_t    time;
    double      x, y;
};

struct pepper_pointer_button_event
{
    uint32_t    time;
    uint32_t    button;
    uint32_t    state;
};

struct pepper_pointer_axis_event
{
    uint32_t    time;
    uint32_t    axis;
    double      value;
};

struct pepper_keyboard_key_event
{
    uint32_t    time;
    uint32_t    key;
    uint32_t    state;
};

struct pepper_touch_down_event
{
    uint32_t    time;
    uint32_t    id;
    double      x, y;
};

struct pepper_touch_up_event
{
    uint32_t    time;
    uint32_t    id;
};

struct pepper_touch_motion_event
{
    uint32_t    time;
    uint32_t    id;
    double      x, y;
};

/* Generic object functions. */
PEPPER_API pepper_object_type_t
pepper_object_get_type(pepper_object_t *object);

PEPPER_API void
pepper_object_set_user_data(pepper_object_t *object, const void *key, void *data,
                            pepper_free_func_t free_func);

PEPPER_API void *
pepper_object_get_user_data(pepper_object_t *object, const void *key);

PEPPER_API pepper_event_listener_t *
pepper_object_add_event_listener(pepper_object_t *object, uint32_t id, int priority,
                                 pepper_event_callback_t callback, void *data);

PEPPER_API void
pepper_event_listener_remove(pepper_event_listener_t *listener);

PEPPER_API void
pepper_event_listener_set_priority(pepper_event_listener_t *listener, int priority);

PEPPER_API void
pepper_object_emit_event(pepper_object_t *object, uint32_t id, void *info);

/* Compositor functions. */
PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name);

PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor);

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor);

PEPPER_API const pepper_list_t *
pepper_compositor_get_output_list(pepper_compositor_t *compositor);

PEPPER_API pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output);

PEPPER_API pepper_view_t *
pepper_compositor_pick_view(pepper_compositor_t *compositor, int32_t x, int32_t y);

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

PEPPER_API const char *
pepper_output_get_name(pepper_output_t *output);

PEPPER_API pepper_output_t *
pepper_compositor_find_output(pepper_compositor_t *compositor, const char *name);

/* Input. */
PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor,
                           const char *seat_name,
                           void *data);

PEPPER_API void
pepper_seat_destroy(pepper_seat_t *seat);

PEPPER_API const char *
pepper_seat_get_name(pepper_seat_t *seat);

PEPPER_API pepper_pointer_t *
pepper_seat_get_pointer(pepper_seat_t *seat);

PEPPER_API pepper_keyboard_t *
pepper_seat_get_keyboard(pepper_seat_t *seat);

PEPPER_API pepper_touch_t *
pepper_seat_get_touch(pepper_seat_t *seat);

PEPPER_API void
pepper_seat_add_input_device(pepper_seat_t *seat, pepper_input_device_t *device);

PEPPER_API void
pepper_seat_remove_input_device(pepper_seat_t *seat, pepper_input_device_t *device);

PEPPER_API void
pepper_pointer_set_position(pepper_pointer_t *pointer, int32_t x, int32_t y);

PEPPER_API void
pepper_pointer_get_position(pepper_pointer_t *pointer, int32_t *x, int32_t *y);

PEPPER_API pepper_view_t *
pepper_pointer_get_focus_view(pepper_pointer_t *pointer);

PEPPER_API void
pepper_pointer_set_focus_view(pepper_pointer_t *pointer, pepper_view_t *view);

PEPPER_API void
pepper_pointer_send_leave(pepper_pointer_t *pointer, pepper_view_t *target_view);

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, pepper_view_t *target_view);

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, pepper_view_t *target_view,
                           uint32_t time, int32_t x, int32_t y);
PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, pepper_view_t *target_view,
                           uint32_t time, uint32_t button, uint32_t state);

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, pepper_view_t *target_view,
                         uint32_t time, uint32_t axis, uint32_t amount);

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus_view(pepper_keyboard_t *keyboard);

PEPPER_API void
pepper_keyboard_set_focus_view(pepper_keyboard_t *keyboard, pepper_view_t *view);

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard, pepper_view_t *target_view);

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard, pepper_view_t *target_view);

PEPPER_API const char *
pepper_input_device_get_property(pepper_input_device_t *device, const char *key);

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
