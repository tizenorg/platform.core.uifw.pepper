#ifndef LIBINPUT_INTERNAL_H
#define LIBINPUT_INTERNAL_H

#include <common.h>
#include "pepper-libinput.h"

typedef struct libinput_seat         libinput_seat_t;

struct pepper_libinput
{
    pepper_compositor_t        *compositor;
    struct udev                *udev;
    struct libinput            *libinput;
    struct wl_event_source     *libinput_event_source;
    int                         libinput_fd;

    struct wl_list              seat_list;
};

struct libinput_seat
{
    pepper_seat_t              *base;
    pepper_libinput_t          *input;

    uint32_t                    id;
    uint32_t                    caps;
    char                       *name;

    int                         pointer_x_last;
    int                         pointer_y_last;
    int                         touch_x_last;   /* FIXME */
    int                         touch_y_last;   /* FIXME */

    struct wl_list              link;
    struct wl_signal            capabilities_signal;
    struct wl_signal            name_signal;
};

#endif /* LIBINPUT_INTERNAL_H */
