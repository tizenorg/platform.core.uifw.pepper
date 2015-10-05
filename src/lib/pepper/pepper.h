#ifndef PEPPER_H
#define PEPPER_H

#include <pepper-utils.h>

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#include <time.h>
#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>

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

typedef struct pepper_input_device      pepper_input_device_t;

typedef struct pepper_event_listener    pepper_event_listener_t;
typedef void (*pepper_event_callback_t)(pepper_event_listener_t *listener, pepper_object_t *object,
                                        uint32_t id, void *info, void *data);

typedef struct pepper_input_event       pepper_input_event_t;

typedef struct pepper_pointer_grab      pepper_pointer_grab_t;
typedef struct pepper_keyboard_grab     pepper_keyboard_grab_t;
typedef struct pepper_touch_grab        pepper_touch_grab_t;

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

enum pepper_output_mode_flag
{
    PEPPER_OUTPUT_MODE_INVALID      = (1 << 0),
    PEPPER_OUTPUT_MODE_CURRENT      = (1 << 1),
    PEPPER_OUTPUT_MODE_PREFERRED    = (1 << 2),
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

enum pepper_built_in_events
{
    PEPPER_EVENT_ALL,
    PEPPER_EVENT_OBJECT_DESTROY,

    PEPPER_EVENT_COMPOSITOR_OUTPUT_ADD,
    PEPPER_EVENT_COMPOSITOR_OUTPUT_REMOVE,
    PEPPER_EVENT_COMPOSITOR_SEAT_ADD,
    PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,
    PEPPER_EVENT_COMPOSITOR_SURFACE_ADD,
    PEPPER_EVENT_COMPOSITOR_SURFACE_REMOVE,
    PEPPER_EVENT_COMPOSITOR_VIEW_ADD,
    PEPPER_EVENT_COMPOSITOR_VIEW_REMOVE,
    PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,
    PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,

    PEPPER_EVENT_OUTPUT_MODE_CHANGE,
    PEPPER_EVENT_OUTPUT_MOVE,

    PEPPER_EVENT_SURFACE_COMMIT,

    PEPPER_EVENT_BUFFER_RELEASE,

    PEPPER_EVENT_VIEW_STACK_CHANGE,

    PEPPER_EVENT_SEAT_POINTER_ADD,
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

    PEPPER_EVENT_FOCUS_ENTER,
    PEPPER_EVENT_FOCUS_LEAVE,

    PEPPER_EVENT_POINTER_MOTION,
    PEPPER_EVENT_POINTER_MOTION_ABSOLUTE,
    PEPPER_EVENT_POINTER_BUTTON,
    PEPPER_EVENT_POINTER_AXIS,

    PEPPER_EVENT_KEYBOARD_KEY,

    PEPPER_EVENT_TOUCH_DOWN,
    PEPPER_EVENT_TOUCH_UP,
    PEPPER_EVENT_TOUCH_MOTION,
    PEPPER_EVENT_TOUCH_FRAME,
    PEPPER_EVENT_TOUCH_CANCEL,
};

enum pepper_pointer_axis
{
    PEPPER_POINTER_AXIS_VERTICAL,
    PEPPER_POINTER_AXIS_HORIZONTAL,
};

enum pepper_button_state
{
    PEPPER_BUTTON_STATE_RELEASED,
    PEPPER_BUTTON_STATE_PRESSED,
};

enum pepper_key_state
{
    PEPPER_KEY_STATE_RELEASED,
    PEPPER_KEY_STATE_PRESSED,
};

struct pepper_input_event
{
    uint32_t    id;
    uint32_t    time;

    uint32_t    button;
    uint32_t    state;
    uint32_t    axis;
    uint32_t    key;
    uint32_t    slot;
    double      x, y;
    double      value;
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

PEPPER_API struct wl_list *
pepper_compositor_get_resource_list(pepper_compositor_t *compositor);

PEPPER_API const char *
pepper_compositor_get_socket_name(pepper_compositor_t *compositor);

PEPPER_API const pepper_list_t *
pepper_compositor_get_output_list(pepper_compositor_t *compositor);

PEPPER_API const pepper_list_t *
pepper_compositor_get_surface_list(pepper_compositor_t *compositor);

PEPPER_API const pepper_list_t *
pepper_compositor_get_view_list(pepper_compositor_t *compositor);

PEPPER_API const pepper_list_t *
pepper_compositor_get_seat_list(pepper_compositor_t *compositor);

PEPPER_API const pepper_list_t *
pepper_compositor_get_input_device_list(pepper_compositor_t *compositor);

PEPPER_API pepper_view_t *
pepper_compositor_pick_view(pepper_compositor_t *compositor,
                            double x, double y, double *vx, double *vy);

PEPPER_API pepper_bool_t
pepper_compositor_set_clock_id(pepper_compositor_t *compositor, clockid_t id);

PEPPER_API pepper_bool_t
pepper_compositor_get_time(pepper_compositor_t *compositor, struct timespec *ts);

/* Output. */
PEPPER_API void
pepper_output_destroy(pepper_output_t *output);

PEPPER_API struct wl_list *
pepper_output_get_resource_list(pepper_output_t *output);

PEPPER_API pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output);

PEPPER_API void
pepper_output_move(pepper_output_t *output, int32_t x, int32_t y);

PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_output_t *output);

PEPPER_API unsigned int
pepper_output_get_scale(pepper_output_t *output);

PEPPER_API int
pepper_output_get_mode_count(pepper_output_t *output);

PEPPER_API void
pepper_output_get_mode(pepper_output_t *output, int index, pepper_output_mode_t *mode);

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_output_t *output, const pepper_output_mode_t *mode);

PEPPER_API const char *
pepper_output_get_name(pepper_output_t *output);

PEPPER_API pepper_output_t *
pepper_compositor_find_output(pepper_compositor_t *compositor, const char *name);

/* Seat & Input Device. */
PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor, const char *seat_name);

PEPPER_API void
pepper_seat_destroy(pepper_seat_t *seat);

PEPPER_API struct wl_list *
pepper_seat_get_resource_list(pepper_seat_t *seat);

PEPPER_API pepper_compositor_t *
pepper_seat_get_compositor(pepper_seat_t *seat);

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

PEPPER_API const char *
pepper_input_device_get_property(pepper_input_device_t *device, const char *key);

PEPPER_API pepper_compositor_t *
pepper_input_device_get_compositor(pepper_input_device_t *device);

/* Pointer. */
struct pepper_pointer_grab
{
    void (*motion)(pepper_pointer_t *pointer, void *data, uint32_t time, double x, double y);
    void (*button)(pepper_pointer_t *pointer, void *data, uint32_t time, uint32_t button,
                   uint32_t state);
    void (*axis)(pepper_pointer_t *pointer, void *data, uint32_t time, uint32_t axis, double value);
    void (*cancel)(pepper_pointer_t *pointer, void *data);
};

PEPPER_API struct wl_list *
pepper_pointer_get_resource_list(pepper_pointer_t *pointer);

PEPPER_API pepper_compositor_t *
pepper_pointer_get_compositor(pepper_pointer_t *pointer);

PEPPER_API pepper_seat_t *
pepper_pointer_get_seat(pepper_pointer_t *pointer);

PEPPER_API pepper_bool_t
pepper_pointer_set_clamp(pepper_pointer_t *pointer, double x0, double y0, double x1, double y1);

PEPPER_API void
pepper_pointer_get_clamp(pepper_pointer_t *pointer, double *x0, double *y0, double *x1, double *y1);

PEPPER_API void
pepper_pointer_set_velocity(pepper_pointer_t *pointer, double vx, double vy);

PEPPER_API void
pepper_pointer_get_velocity(pepper_pointer_t *pointer, double *vx, double *vy);

PEPPER_API void
pepper_pointer_get_position(pepper_pointer_t *pointer, double *x, double *y);

PEPPER_API void
pepper_pointer_set_focus(pepper_pointer_t *pointer, pepper_view_t *focus);

PEPPER_API pepper_view_t *
pepper_pointer_get_focus(pepper_pointer_t *pointer);

PEPPER_API void
pepper_pointer_send_leave(pepper_pointer_t *pointer);

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, double x, double y);

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, uint32_t time, double x, double y);

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, uint32_t time, uint32_t button, uint32_t state);

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, uint32_t time, uint32_t axis, double value);

PEPPER_API void
pepper_pointer_set_grab(pepper_pointer_t *pointer, const pepper_pointer_grab_t *grab, void *data);

PEPPER_API const pepper_pointer_grab_t *
pepper_pointer_get_grab(pepper_pointer_t *pointer);

PEPPER_API void *
pepper_pointer_get_grab_data(pepper_pointer_t *pointer);

/* Keyboard. */
struct pepper_keyboard_grab
{
    void (*key)(pepper_keyboard_t *keyboard, void *data, uint32_t time, uint32_t key, uint32_t state);
    void (*cancel)(pepper_keyboard_t *keyboard, void *data);
};

PEPPER_API struct wl_list *
pepper_keyboard_get_resource_list(pepper_keyboard_t *keyboard);

PEPPER_API pepper_compositor_t *
pepper_keyboard_get_compositor(pepper_keyboard_t *keyboard);

PEPPER_API pepper_seat_t *
pepper_keyboard_get_seat(pepper_keyboard_t *keyboard);

PEPPER_API void
pepper_keyboard_set_focus(pepper_keyboard_t *keyboard, pepper_view_t *focus);

PEPPER_API pepper_view_t *
pepper_keyboard_get_focus(pepper_keyboard_t *keyboard);

PEPPER_API void
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard);

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard);

PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, uint32_t time, uint32_t key, uint32_t state);

PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard, uint32_t depressed, uint32_t latched,
                               uint32_t locked, uint32_t group);

PEPPER_API void
pepper_keyboard_set_grab(pepper_keyboard_t *keyboard, const pepper_keyboard_grab_t *grab, void *data);

PEPPER_API const pepper_keyboard_grab_t *
pepper_keyboard_get_grab(pepper_keyboard_t *keyboard);

PEPPER_API void *
pepper_keyboard_get_grab_data(pepper_keyboard_t *keyboard);

PEPPER_API void
pepper_keyboard_set_keymap(pepper_keyboard_t *keyboard, struct xkb_keymap *keymap);

/* Touch. */
struct pepper_touch_grab
{
    void    (*down)(pepper_touch_t *touch, void *data, uint32_t time, int32_t id, double x, double y);
    void    (*up)(pepper_touch_t *touch, void *data, uint32_t time, uint32_t id);
    void    (*motion)(pepper_touch_t *touch, void *data,
                      uint32_t time, uint32_t id, double x, double y);
    void    (*frame)(pepper_touch_t *touch, void *data);
    void    (*cancel)(pepper_touch_t *touch, void *data);
};

PEPPER_API struct wl_list *
pepper_touch_get_resource_list(pepper_touch_t *touch);

PEPPER_API pepper_compositor_t *
pepper_touch_get_compositor(pepper_touch_t *touch);

PEPPER_API pepper_seat_t *
pepper_touch_get_seat(pepper_touch_t *touch);

PEPPER_API void
pepper_touch_set_focus(pepper_touch_t *touch, pepper_view_t *focus);

PEPPER_API pepper_view_t *
pepper_touch_get_focus(pepper_touch_t *touch);

PEPPER_API void
pepper_touch_send_down(pepper_touch_t *touch, uint32_t time, uint32_t id, double x, double y);

PEPPER_API void
pepper_touch_send_up(pepper_touch_t *touch, uint32_t time, uint32_t id);

PEPPER_API void
pepper_touch_send_motion(pepper_touch_t *touch, uint32_t time, uint32_t id, double x, double y);

PEPPER_API void
pepper_touch_send_frame(pepper_touch_t *touch);

PEPPER_API void
pepper_touch_send_cancel(pepper_touch_t *touch);

PEPPER_API void
pepper_touch_set_grab(pepper_touch_t *touch, const pepper_touch_grab_t *grab, void *data);

PEPPER_API const pepper_touch_grab_t *
pepper_touch_get_grab(pepper_touch_t *touch);

PEPPER_API void *
pepper_touch_get_grab_data(pepper_touch_t *touch);

/* Surface. */
PEPPER_API struct wl_resource *
pepper_surface_get_resource(pepper_surface_t *surface);

PEPPER_API pepper_compositor_t *
pepper_surface_get_compositor(pepper_surface_t *surface);

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

PEPPER_API pixman_region32_t *
pepper_surface_get_damage_region(pepper_surface_t *surface);

PEPPER_API pixman_region32_t *
pepper_surface_get_opaque_region(pepper_surface_t *surface);

PEPPER_API pixman_region32_t *
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
pepper_compositor_add_view(pepper_compositor_t *compositor);

PEPPER_API pepper_bool_t
pepper_view_set_surface(pepper_view_t *view, pepper_surface_t *surface);

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

PEPPER_API pepper_bool_t
pepper_view_is_opaque(pepper_view_t *view);

PEPPER_API void
pepper_view_get_local_coordinate(pepper_view_t *view, double gx, double gy, double *lx, double *ly);

PEPPER_API void
pepper_view_get_global_coordinate(pepper_view_t *view, double lx, double ly, double *gx, double *gy);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
