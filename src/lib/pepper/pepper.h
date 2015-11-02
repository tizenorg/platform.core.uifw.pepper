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

/**
 * @typedef pepper_object_t
 *
 * A #pepper_object_t is a generic object providing common functionalities of
 * all other pepper objects such as event handling, caching user data and etc.
 *
 * Following is the list of pepper objects inheriting #pepper_object_t
 *  - #pepper_compositor_t
 *  - #pepper_output_t
 *  - #pepper_surface_t
 *  - #pepper_buffer_t
 *  - #pepper_view_t
 *  - #pepper_seat_t
 *  - #pepper_pointer_t
 *  - #pepper_keyboard_t
 *  - #pepper_touch_t
 *  - #pepper_input_device_t
 *  - #pepper_plane_t (used internally)
 *
 * Casting the above objects into #pepper_object_t is safe.
 */
typedef struct pepper_object            pepper_object_t;

/**
 * @typedef pepper_compositor_t
 *
 * A #pepper_compositor_t represents an instance of a wayland compositor which
 * has a wl_display listening on a given socket. All other pepper objects have
 * their belonging #pepper_compositor_t.
 */
typedef struct pepper_compositor        pepper_compositor_t;

/**
 * @typedef pepper_output_t
 *
 * A #pepper_output_t represents a wl_output.
 */
typedef struct pepper_output            pepper_output_t;

/**
 * @typedef pepper_surface_t
 *
 * A #pepper_surface_t represents a wl_surface.
 */
typedef struct pepper_surface           pepper_surface_t;

/**
 * @typedef pepper_buffer_t
 *
 * A #pepper_buffer_t represents a wl_buffer.
 */
typedef struct pepper_buffer            pepper_buffer_t;

/**
 * @typedef pepper_view_t
 *
 * A #pepper_view_t represents a visible rectangular shaped object on the
 * compositor coordinate space. There're several types of a #pepper_view_t.
 *
 * surface view  : A #pepper_view_t having a #pepper_surface_t as its content
 * grouping view : A #pepper_view_t having no attached content
 *
 * Views can be constructed as a tree hierarchy. The global position and
 * transform of a child view is determined relative to its parent's according
 * to the inheritance flag.
 *
 * A #pepper_compositor_t maintains a list of views in sorted z-order and they
 * are drawn back to front using painter's algorithm.
 */
typedef struct pepper_view              pepper_view_t;

/**
 * @typedef pepper_seat_t
 *
 * A #pepper_seat_t represents a wl_seat.
 *
 * #pepper_input_device_t can be added to a #pepper_seat_t so that it can
 * receive input events from the device and dispatch them to the
 * #pepper_pointer_t/#pepper_keyboard_t/#pepper_touch_t. Adding or removing
 * a device might result change in seat's capability.
 */
typedef struct pepper_seat              pepper_seat_t;

/**
 * @typedef pepper_pointer_t
 *
 * A #pepper_pointer_t represents a wl_pointer.
 */
typedef struct pepper_pointer           pepper_pointer_t;

/**
 * @typedef pepper_keyboard_t
 *
 * A #pepper_keyboard_t represents a wl_keyboard.
 */
typedef struct pepper_keyboard          pepper_keyboard_t;

/**
 * @typedef pepper_touch_t
 *
 * A #pepper_touch_t represents a wl_touch.
 */
typedef struct pepper_touch             pepper_touch_t;

/**
 * @typedef pepper_output_geometry_t
 *
 * A #pepper_output_geometry_t is a data structure holding geometry information
 * of an wl_output.
 */
typedef struct pepper_output_geometry   pepper_output_geometry_t;

/**
 * @typedef pepper_output_mode_t
 *
 * A #pepper_output_mode_t is a data structure for output mode information.
 */
typedef struct pepper_output_mode       pepper_output_mode_t;

/**
 * @typedef pepper_input_device_t
 *
 * A #pepper_input_device_t is a device generating input events and it is
 * created the input backend. An input device may have several capabilities like
 * pointer + keyboard and any value passed from #pepper_input_device_t is device
 * specific such as pointer motion value or touch position.
 */
typedef struct pepper_input_device      pepper_input_device_t;

/**
 * @typedef pepper_event_listener_t
 *
 * A #pepper_event_listener_t is a handle to a event listener.
 */
typedef struct pepper_event_listener    pepper_event_listener_t;

/**
 * @typedef pepper_event_callback_t
 *
 * Function pointer to be invoked when the desired event is emitted on the
 * given #pepper_object_t.
 *
 * @param listener  listener object.
 * @param object    object on which the event has been emitted.
 * @param id        event id (See #pepper_built_in_events).
 * @param info      information of the event.
 * @param data      data passed when adding the event listener.
 */
typedef void (*pepper_event_callback_t)(pepper_event_listener_t *listener, pepper_object_t *object,
                                        uint32_t id, void *info, void *data);

/**
 * @typedef pepper_input_event_t
 *
 * A #pepper_input_event_t is a data structure for holding information about
 * various input events (pointer/keyboard/touch).
 */
typedef struct pepper_input_event       pepper_input_event_t;

/**
 * @typedef pepper_pointer_grab_t
 *
 * A #pepper_pointer_grab_t is a set of function pointers which are invoked
 * when the corresponding input events are emitted on the #pepper_pointer_t.
 *
 * @see pepper_pointer_set_grab()
 * @see pepper_pointer_get_grab()
 */
typedef struct pepper_pointer_grab      pepper_pointer_grab_t;

/**
 * @typedef pepper_keyboard_grab_t
 *
 * A #pepper_keyboard_grab_t is a set of function keyboards which are invoked
 * when the corresponding input events are emitted on the #pepper_keyboard_t.
 *
 * @see pepper_keyboard_set_grab()
 * @see pepper_keyboard_get_grab()
 */
typedef struct pepper_keyboard_grab     pepper_keyboard_grab_t;

/**
 * @typedef pepper_touch_grab_t
 *
 * A #pepper_touch_grab_t is a set of function touchs which are invoked
 * when the corresponding input events are emitted on the #pepper_touch_t.
 *
 * @see pepper_touch_set_grab()
 * @see pepper_touch_get_grab()
 */
typedef struct pepper_touch_grab        pepper_touch_grab_t;

struct pepper_output_geometry
{
    int32_t     x;          /**< x coordinate of the output on the compositor coordinate space. */
    int32_t     y;          /**< y coordinate of the output on the compositor coordinate space. */
    int32_t     w;          /**< width of the output. */
    int32_t     h;          /**< height of the output. */
    int32_t     subpixel;   /**< subpixel layout of the output. */
    const char  *maker;     /**< maker of the output. */
    const char  *model;     /**< model name of the output. */
    int32_t     transform;  /**< wl_output::transform. */
};

struct pepper_output_mode
{
    uint32_t    flags;      /**< bit flag #pepper_outout_mode_flag. */
    int32_t     w, h;       /**< width and height of the output mode. */
    int32_t     refresh;    /**< refresh rate. */
};

enum pepper_output_mode_flag
{
    PEPPER_OUTPUT_MODE_INVALID      = (1 << 0), /**< the mode is invalid. */
    PEPPER_OUTPUT_MODE_CURRENT      = (1 << 1), /**< the mode is current mode. */
    PEPPER_OUTPUT_MODE_PREFERRED    = (1 << 2), /**< the mode is preferred mode. */
};

typedef enum pepper_object_type
{
    PEPPER_OBJECT_COMPOSITOR,   /**< #pepper_compositor_t */
    PEPPER_OBJECT_OUTPUT,       /**< #pepper_output_t */
    PEPPER_OBJECT_SURFACE,      /**< #pepper_surface_t */
    PEPPER_OBJECT_BUFFER,       /**< #pepper_buffer_t */
    PEPPER_OBJECT_VIEW,         /**< #pepper_view_t */
    PEPPER_OBJECT_SEAT,         /**< #pepper_seat_t */
    PEPPER_OBJECT_POINTER,      /**< #pepper_pointer_t */
    PEPPER_OBJECT_KEYBOARD,     /**< #pepper_keyboard_t */
    PEPPER_OBJECT_TOUCH,        /**< #pepper_touch_t */
    PEPPER_OBJECT_INPUT_DEVICE, /**< #pepper_input_device_t */
    PEPPER_OBJECT_PLANE,        /**< #pepper_plane_t (internally used) */
} pepper_object_type_t;

enum pepper_built_in_events
{
    /**
     * All events of a #pepper_object_t
     *
     * #pepper_object_t
     *  - when : Any event is emitted
     *  - info : corresponding info of the event
     */
    PEPPER_EVENT_ALL,

    /**
     * Destruction of a #pepper_object_t
     *
     * #pepper_object_t
     * - when : #pepper_object_t is about to be destroyed
     * - info : NULL
     */
    PEPPER_EVENT_OBJECT_DESTROY,

    /**
     * Addition of a #pepper_output_t to a #pepper_compositor_t
     *
     * #pepper_compositor_t
     *  - when : #pepper_output_t has been added
     *  - info : the added #pepper_output_t
     */
    PEPPER_EVENT_COMPOSITOR_OUTPUT_ADD,

    /**
     * Removal of a #pepper_output_t from a #pepper_compositor_t
     *
     * #pepper_compositor_t
     *  - when : #pepper_output_t has been removed
     *  - info : the removed #pepper_output_t.
     */
    PEPPER_EVENT_COMPOSITOR_OUTPUT_REMOVE,

    /**
     * Addition of a #pepper_seat_t to a #pepper_compositor_t
     *
     * #pepper_compositor_t
     * - when : #pepper_seat_t has been added
     * - info : the added #pepper_seat_t
     */
    PEPPER_EVENT_COMPOSITOR_SEAT_ADD,

    /**
     * Removal of a #pepper_seat_t from a #pepper_compositor_t.
     * 
     * #pepper_compositor_t
     *  - when : #pepper_seat_t has been removed
     *  - info : the removed #pepper_seat_t
     */
    PEPPER_EVENT_COMPOSITOR_SEAT_REMOVE,

    /**
     * Addition of a #pepper_surface_t to a #pepper_compositor_t.
     *
     * #pepper_compositor_t
     *  - when : #pepper_surface_t has been added
     *  - info : the added #pepper_surface_t
     */
    PEPPER_EVENT_COMPOSITOR_SURFACE_ADD,

    /**
     * Removal of a #pepper_surface_t from a #pepper_compositor_t.
     *
     * #pepper_compositor_t
     *  - when : #pepper_surface_t has been removed
     *  - info : the removed #pepper_surface_t
     */
    PEPPER_EVENT_COMPOSITOR_SURFACE_REMOVE,

    /**
     * Addition of a #pepper_view_t to a #pepper_compositor_t.
     *
     * #pepper_compositor_t
     *  - when : #pepper_view_t has been added
     *  - info : the added #pepper_view_t
     */
    PEPPER_EVENT_COMPOSITOR_VIEW_ADD,

    /**
     * Removal of a #pepper_view_t from a #pepper_compositor_t.
     *
     * #pepper_compositor_t
     *  - when : #pepper_view_t has been removed
     *  - info : the removed #pepper_view_t
     */
    PEPPER_EVENT_COMPOSITOR_VIEW_REMOVE,

    /**
     * Addition of a #pepper_input_device_t to a #pepper_compositor_t.
     *
     * #pepper_compositor_t
     *  - when : #pepper_input_device_t has been added
     *  - info : the added #pepper_input_device_t
     */
    PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_ADD,

    /**
     * Removal of a #pepper_input_device_t from a #pepper_compositor_t.
     *
     * #pepper_compositor_t
     *  - when : #pepper_input_device_t has been removed
     *  - info : the removed #pepper_input_device_t
     */
    PEPPER_EVENT_COMPOSITOR_INPUT_DEVICE_REMOVE,

    /**
     * Change in current mode of a #pepper_output_t.
     *
     * #pepper_output_t
     * - when : mode of the #pepper_output_t has been changed
     * - info : NULL
     */
    PEPPER_EVENT_OUTPUT_MODE_CHANGE,

    /**
     * Change in position of a #pepper_output_t.
     *
     * #pepper_output_t
     * - when : #pepper_output_t has moved its position.
     * - info : NULL
     */
    PEPPER_EVENT_OUTPUT_MOVE,

    /**
     * wl_surface::commit
     *
     * #pepper_surface_t
     *  - when : wl_surface::commit has been requested
     *  - info : NULL
     */
    PEPPER_EVENT_SURFACE_COMMIT,

    /**
     * wl_buffer::release
     *
     * #pepper_buffer_t
     *  - when : wl_buffer::release has been sent
     *  - info : NULL
     */
    PEPPER_EVENT_BUFFER_RELEASE,

    /**
     * Z-order change of a #pepper_view_t.
     *
     * #pepper_view_t
     *  - when : stack (z-order) has been changed
     *  - info : NULL
     */
    PEPPER_EVENT_VIEW_STACK_CHANGE,

    /**
     * Addition of a #pepper_pointer_t to a #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_pointer_t has been added
     *  - info : the added #pepper_pointer_t
     */
    PEPPER_EVENT_SEAT_POINTER_ADD,

    /**
     * Removal of a #pepper_pointer_t from a #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_pointer_t has been removed
     *  - info : the removed #pepper_pointer_t
     */
    PEPPER_EVENT_SEAT_POINTER_REMOVE,

    /**
     * Addition of a #pepper_keyboard_t to a #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_keyboard_t has been added
     *  - info : the added #pepper_keyboard_t
     */
    PEPPER_EVENT_SEAT_KEYBOARD_ADD,

    /**
     * Removal of a #pepper_keyboard_t from a #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_keyboard_t has been removed
     *  - info : the removed #pepper_keyboard_t
     */
    PEPPER_EVENT_SEAT_KEYBOARD_REMOVE,

    /**
     * Addition of a #pepper_touch_t to a #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_touch_t has been added
     *  - info : the added #pepper_touch_t
     */
    PEPPER_EVENT_SEAT_TOUCH_ADD,

    /**
     * Removal of a #pepper_touch_t from a #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_touch_t has been removed
     *  - info : the removed #pepper_touch_t
     */
    PEPPER_EVENT_SEAT_TOUCH_REMOVE,

    /**
     * Addition of a #pepper_input_device_t having pointer capability to a
     * #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_input_device_t having pointer capability has been added
     *  - info : the added #pepper_input_device_t
     */
    PEPPER_EVENT_SEAT_POINTER_DEVICE_ADD,

    /**
     * Removal of a #pepper_input_device_t having pointer capability from a
     * #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_input_device_t having pointer capability has been removed
     *  - info : the removed #pepper_input_device_t
     */
    PEPPER_EVENT_SEAT_POINTER_DEVICE_REMOVE,

    /**
     * Addition of a #pepper_input_device_t having keyboard capability to a
     * #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_input_device_t having keyboard capability has been added
     *  - info : the added #pepper_input_device_t
     */
    PEPPER_EVENT_SEAT_KEYBOARD_DEVICE_ADD,

    /**
     * Removal of a #pepper_input_device_t having keyboard capability from a
     * #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_input_device_t having keyboard capability has been removed
     *  - info : the removed #pepper_input_device_t
     */
    PEPPER_EVENT_SEAT_KEYBOARD_DEVICE_REMOVE,

    /**
     * Addition of a #pepper_input_device_t having touch capability to a
     * #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_input_device_t having touch capability has been added
     *  - info : the added #pepper_input_device_t
     */
    PEPPER_EVENT_SEAT_TOUCH_DEVICE_ADD,

    /**
     * Removal of a #pepper_input_device_t having touch capability from a
     * #pepper_seat_t.
     *
     * #pepper_seat_t
     *  - when : #pepper_input_device_t having touch capability has been removed
     *  - info : the removed #pepper_input_device_t
     */
    PEPPER_EVENT_SEAT_TOUCH_DEVICE_REMOVE,

    /**
     * #pepper_view_t get an input focus.
     *
     * #pepper_pointer_t
     *  - when : #pepper_view_t has been set for the focus
     *  - info : the focused #pepper_view_t
     *
     * #pepper_keyboard_t
     *  - when : #pepper_view_t has been set for the focus
     *  - info : the focused #pepper_view_t
     *
     * #pepper_touch_t
     *  - when : #pepper_view_t has been set for the focus
     *  - info : the focused #pepper_view_t
     *
     * #pepper_view_t
     *  - when : Get focused for any of #pepper_pointer_t/#pepper_keyboard_t/#pepper_touch_t
     *  - info : #pepper_pointer_t/#pepper_keyboard_t/#pepper_touch_t
     */
    PEPPER_EVENT_FOCUS_ENTER,

    /**
     * #pepper_view_t loses an input focus.
     *
     * #pepper_pointer_t
     *  - when : current focus has been changed
     *  - info : previously focused #pepper_view_t
     *
     * #pepper_keyboard_t
     *  - when : current focus has been changed
     *  - info : previously focused #pepper_view_t
     *
     * #pepper_touch_t
     *  - when : current focus has been changed
     *  - info : previously focused #pepper_view_t
     *
     * #pepper_view_t
     *  - when : loses any focus of #pepper_pointer_t/#pepper_keyboard_t/#pepper_touch_t
     *  - info : #pepper_pointer_t/#pepper_keyboard_t/#pepper_touch_t
     */
    PEPPER_EVENT_FOCUS_LEAVE,

    /**
     * Relative pointer motion event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits pointer motion event
     *  - info : #pepper_input_event_t
     *
     * #pepper_pointer_t
     *  - when : #pepper_input_device_t emits pointer motion event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_POINTER_MOTION,

    /**
     * Absolute pointer motion event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits pointer motion event
     *  - info : #pepper_input_event_t
     *
     * #pepper_pointer_t
     *  - when : #pepper_input_device_t emits pointer motion event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_POINTER_MOTION_ABSOLUTE,

    /**
     * Pointer button event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits pointer button event
     *  - info : #pepper_input_event_t
     *
     * #pepper_pointer_t
     *  - when : #pepper_input_device_t emits pointer button event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_POINTER_BUTTON,

    /**
     * Pointer axis event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits pointer axis event
     *  - info : #pepper_input_event_t
     *
     * #pepper_pointer_t
     *  - when : #pepper_input_device_t emits pointer axis event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_POINTER_AXIS,

    /**
     * Keyboard key event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits key event
     *  - info : #pepper_input_event_t
     *
     * #pepper_keyboard_t
     *  - when : #pepper_input_device_t emits key event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_KEYBOARD_KEY,

    /**
     * Touch down event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits touch down event
     *  - info : #pepper_input_event_t
     *
     * #pepper_touch_t
     *  - when : #pepper_input_device_t emits touch down event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_TOUCH_DOWN,

    /**
     * Touch up event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits touch up event
     *  - info : #pepper_input_event_t
     *
     * #pepper_touch_t
     *  - when : #pepper_input_device_t emits touch up event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_TOUCH_UP,

    /**
     * Touch motion event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits touch motion event
     *  - info : #pepper_input_event_t
     *
     * #pepper_touch_t
     *  - when : #pepper_input_device_t emits touch motion event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_TOUCH_MOTION,

    /**
     * Touch frame event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits touch frame event
     *  - info : #pepper_input_event_t
     *
     * #pepper_touch_t
     *  - when : #pepper_input_device_t emits touch frame event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_TOUCH_FRAME,

    /**
     * Touch calcen event.
     *
     * #pepper_input_device_t
     *  - when : input backend emits touch cancel event
     *  - info : #pepper_input_event_t
     *
     * #pepper_touch_t
     *  - when : #pepper_input_device_t emits touch cancel event (to the attached #pepper_seat_t)
     *  - info : #pepper_input_event_t
     */
    PEPPER_EVENT_TOUCH_CANCEL,
};

enum pepper_pointer_axis
{
    PEPPER_POINTER_AXIS_VERTICAL,   /**< vertical pointer axis. */
    PEPPER_POINTER_AXIS_HORIZONTAL, /**< horizontal pointer axis. */
};

enum pepper_button_state
{
    PEPPER_BUTTON_STATE_RELEASED,   /**< button is relased. */
    PEPPER_BUTTON_STATE_PRESSED,    /**< button is pressed. */
};

enum pepper_key_state
{
    PEPPER_KEY_STATE_RELEASED,      /**< key is released. */
    PEPPER_KEY_STATE_PRESSED,       /**< key is pressed. */
};

struct pepper_input_event
{
    uint32_t    id;         /**< event id #pepper_built_in_events */
    uint32_t    time;       /**< time in mili-second with undefined base. */

    uint32_t    button;     /**< pointer button flag. */
    uint32_t    state;      /**< pointer and key state flag. */
    uint32_t    axis;       /**< pointer axis. */
    uint32_t    key;        /**< keyboard key. */
    uint32_t    slot;       /**< touch point id. */
    double      x, y;       /**< x, y coordinate value. */
    double      value;      /**< pointer axis value. */
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
pepper_pointer_send_leave(pepper_pointer_t *pointer, pepper_view_t *view);

PEPPER_API void
pepper_pointer_send_enter(pepper_pointer_t *pointer, pepper_view_t *view, double x, double y);

PEPPER_API void
pepper_pointer_send_motion(pepper_pointer_t *pointer, pepper_view_t *view,
                           uint32_t time, double x, double y);

PEPPER_API void
pepper_pointer_send_button(pepper_pointer_t *pointer, pepper_view_t *view,
                           uint32_t time, uint32_t button, uint32_t state);

PEPPER_API void
pepper_pointer_send_axis(pepper_pointer_t *pointer, pepper_view_t *view,
                         uint32_t time, uint32_t axis, double value);

PEPPER_API void
pepper_pointer_set_grab(pepper_pointer_t *pointer, const pepper_pointer_grab_t *grab, void *data);

PEPPER_API const pepper_pointer_grab_t *
pepper_pointer_get_grab(pepper_pointer_t *pointer);

PEPPER_API void *
pepper_pointer_get_grab_data(pepper_pointer_t *pointer);

/* Keyboard. */
struct pepper_keyboard_grab
{
    void (*key)(pepper_keyboard_t *keyboard, void *data, uint32_t time, uint32_t key,
                uint32_t state);
    void (*modifiers)(pepper_keyboard_t *keyboard, void *data, uint32_t mods_depressed,
                      uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
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
pepper_keyboard_send_leave(pepper_keyboard_t *keyboard, pepper_view_t *view);

PEPPER_API void
pepper_keyboard_send_enter(pepper_keyboard_t *keyboard, pepper_view_t *view);

PEPPER_API void
pepper_keyboard_send_key(pepper_keyboard_t *keyboard, pepper_view_t *view,
                         uint32_t time, uint32_t key, uint32_t state);

PEPPER_API void
pepper_keyboard_send_modifiers(pepper_keyboard_t *keyboard, pepper_view_t *view,
                               uint32_t depressed, uint32_t latched,
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
    void    (*cancel_touch_point)(pepper_touch_t *touch, uint32_t id, void *data);
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
pepper_touch_add_point(pepper_touch_t *touch, uint32_t id, double x, double y);

PEPPER_API void
pepper_touch_remove_point(pepper_touch_t *touch, uint32_t id);

PEPPER_API void
pepper_touch_point_set_focus(pepper_touch_t *touch, uint32_t id, pepper_view_t *focus);

PEPPER_API pepper_view_t *
pepper_touch_point_get_focus(pepper_touch_t *touch, uint32_t id);

PEPPER_API void
pepper_touch_point_get_position(pepper_touch_t *touch, uint32_t id, double *x, double *y);

PEPPER_API void
pepper_touch_send_down(pepper_touch_t *touch, pepper_view_t *view,
                       uint32_t time, uint32_t id, double x, double y);

PEPPER_API void
pepper_touch_send_up(pepper_touch_t *touch, pepper_view_t *view, uint32_t time, uint32_t id);

PEPPER_API void
pepper_touch_send_motion(pepper_touch_t *touch, pepper_view_t *view,
                         uint32_t time, uint32_t id, double x, double y);

PEPPER_API void
pepper_touch_send_frame(pepper_touch_t *touch, pepper_view_t *view);

PEPPER_API void
pepper_touch_send_cancel(pepper_touch_t *touch, pepper_view_t *view);

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

PEPPER_API pepper_bool_t
pepper_surface_get_keep_buffer(pepper_surface_t *surface);

PEPPER_API void
pepper_surface_set_keep_buffer(pepper_surface_t *surface, pepper_bool_t keep_buffer);

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

PEPPER_API void
pepper_view_set_transform_inherit(pepper_view_t *view, pepper_bool_t inherit);

PEPPER_API pepper_bool_t
pepper_view_get_transform_inherit(pepper_view_t *view);

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
