#ifndef PEPPER_H
#define PEPPER_H

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#   define PEPPER_API __attribute__ ((visibility("default")))
#else
#   define PEPPER_API
#endif

#define PEPPER_FALSE    0
#define PEPPER_TRUE     1

typedef unsigned int                    pepper_bool_t;

#include <pepper-util.h>

typedef struct pepper_compositor        pepper_compositor_t;

typedef struct pepper_output_geometry   pepper_output_geometry_t;
typedef struct pepper_output_mode       pepper_output_mode_t;
typedef struct pepper_output            pepper_output_t;
typedef struct pepper_output_interface  pepper_output_interface_t;

typedef struct pepper_seat_interface    pepper_seat_interface_t;
typedef struct pepper_seat              pepper_seat_t;
typedef struct pepper_pointer           pepper_pointer_t;
typedef struct pepper_keyboard          pepper_keyboard_t;
typedef struct pepper_touch             pepper_touch_t;

typedef struct pepper_input_event       pepper_input_event_t;

typedef struct pepper_renderer          pepper_renderer_t;
typedef struct pepper_surface           pepper_surface_t;

#define PEPPER_FORMAT(type, bpp, a, r, g, b)    \
    ((((type) & 0xff) << 24)    |               \
     (( (bpp) & 0xff) << 16)    |               \
     ((   (a) & 0x0f) << 12)    |               \
     ((   (r) & 0x0f) <<  8)    |               \
     ((   (g) & 0x0f) <<  4)    |               \
     ((   (b) & 0x0f) <<  0))

#define PEPPER_FORMAT_TYPE(format)  (((format) & 0xff000000) >> 24)
#define PEPPER_FORMAT_BPP(format)   (((format) & 0x00ff0000) >> 16)
#define PEPPER_FORMAT_A(format)     (((format) & 0x0000f000) >> 12)
#define PEPPER_FORMAT_R(format)     (((format) & 0x00000f00) >>  8)
#define PEPPER_FORMAT_G(format)     (((format) & 0x000000f0) >>  4)
#define PEPPER_FORMAT_B(format)     (((format) & 0x0000000f) >>  0)

typedef enum
{
    PEPPER_FORMAT_TYPE_UNKNOWN,
    PEPPER_FORMAT_TYPE_ARGB,
    PEPPER_FORMAT_TYPE_ABGR,
} pepper_format_type_t;

typedef enum
{
    PEPPER_FORMAT_UNKNOWN       = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_UNKNOWN,  0,  0,  0,  0,  0),

    PEPPER_FORMAT_ARGB8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    32,  8,  8,  8,  8),
    PEPPER_FORMAT_XRGB8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    32,  0,  8,  8,  8),
    PEPPER_FORMAT_RGB888        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    24,  0,  8,  8,  8),
    PEPPER_FORMAT_RGB565        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    16,  0,  5,  6,  5),

    PEPPER_FORMAT_ABGR8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    32,  8,  8,  8,  8),
    PEPPER_FORMAT_XBGR8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    32,  0,  8,  8,  8),
    PEPPER_FORMAT_BGR888        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    24,  0,  8,  8,  8),
    PEPPER_FORMAT_BGR565        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    16,  0,  5,  6,  5),

    PEPPER_FORMAT_ALPHA         = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,     8,  8,  0,  0,  0),
} pepper_format_t;

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

    void            (*repaint)(void *output);
    void            (*add_frame_listener)(void *output, struct wl_listener *listener);
};

/* Compositor functions. */
PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name);

PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor);

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor);

PEPPER_API pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t *compositor,
                             const pepper_output_interface_t *interface,
                             void *data);

PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor,
                           const pepper_seat_interface_t *interface,
                           void *data);

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
pepper_seat_handle_event(pepper_seat_t *seat, pepper_input_event_t *event);

/* Renderer. */
struct pepper_renderer
{
    void            (*destroy)(pepper_renderer_t *renderer);

    pepper_bool_t   (*read_pixels)(pepper_renderer_t *renderer, void *target,
                                   int x, int y, int w, int h,
                                   void *pixels, pepper_format_t format);

    void            (*draw)(pepper_renderer_t *renderer, void *data, void *target);
};

PEPPER_API void
pepper_renderer_init(pepper_renderer_t *renderer);

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer);

/* Surface. */
PEPPER_API void
pepper_surface_add_destroy_listener(pepper_surface_t *surface, struct wl_listener *listener);

PEPPER_API const char *
pepper_surface_get_role(pepper_surface_t *surface);

PEPPER_API pepper_bool_t
pepper_surface_set_role(pepper_surface_t *surface, const char *role);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
