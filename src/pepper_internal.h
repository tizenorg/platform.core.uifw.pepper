#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include "common.h"
#include "pepper.h"
#include <wayland-server.h>

/* input */
enum pepper_input_event_type
{
    PEPPER_INPUT_POINTER,
    PEPPER_INPUT_KEYBOARD,
    PEPPER_INPUT_TOUCH
};

typedef struct pepper_input_event   pepper_input_event_t;

struct pepper_input_event
{
    enum pepper_input_event_type    type;
    union
    {
        struct
        {
            uint32_t    button;
            uint32_t    time;
            double      x;
            double      y;
        } pointer;

        struct
        {
            uint32_t    key;
            uint32_t    time;
        } keyboard;

        struct
        {
            uint32_t    index;
            uint32_t    time;
            double      x;
            double      y;
        } touch;
    } data;
};

struct pepper_input_module_interface
{
    int     (*get_event_fd)();
    void    (*dispatch_events)();
    int     (*get_next_event)(pepper_input_event_t *, void *);
    /* TODO: */
};
typedef struct pepper_input_module_interface    pepper_input_module_interface_t;

struct pepper_pointer
{
    struct wl_resource *resource;
    /* TODO: */
};

struct pepper_keyboard
{
    struct wl_resource *resource;
    /* TODO: */
};

struct pepper_touch
{
    struct wl_resource *resource;
    /* TODO: */
};

struct pepper_seat
{
    struct wl_resource  *resource;

    char                *name;

    pepper_pointer_t    *pointer;
    pepper_keyboard_t   *keyboard;
    pepper_touch_t      *touch;

    /* TODO: */
};


/* compositor */
struct pepper_compositor
{
    char                            *socket_name;
    struct wl_display               *display;

    pepper_input_module_interface_t input_module_interface;
    void                            *input_module_data;

    pepper_seat_t                   *seat;
};

struct pepper_output
{
    pepper_compositor_t *compositor;

    int x;
    int y;
    int w;
    int h;
};

struct pepper_client
{
    pepper_compositor_t *compositor;
};

struct pepper_surface
{
    struct wl_resource *resource;
    void               *buffer;
};

struct pepper_shell_surface
{
    struct wl_resource *resource;
    pepper_surface_t   *surface;
};
#endif /* PEPPER_INTERNAL_H */
