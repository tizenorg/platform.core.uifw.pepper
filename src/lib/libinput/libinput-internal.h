#ifndef LIBINPUT_INTERNAL_H
#define LIBINPUT_INTERNAL_H

#include "pepper-libinput.h"
#include <pepper-input-backend.h>

/* TODO: Error logging. */
#define PEPPER_ERROR(...)

typedef struct li_device            li_device_t;
typedef struct li_device_property   li_device_property_t;

struct pepper_libinput
{
    pepper_compositor_t        *compositor;
    struct udev                *udev;
    struct libinput            *libinput;
    struct wl_event_source     *libinput_event_source;
    int                         libinput_fd;

    struct wl_list              device_list;
};

struct li_device
{
    pepper_libinput_t          *input;
    pepper_input_device_t      *base;

    uint32_t                    caps;

    struct wl_list              property_list;
    struct wl_list              link;
};

struct li_device_property   /* FIXME */
{
    char                       *key;
    char                       *data;
    struct wl_list              link;
};

#endif /* LIBINPUT_INTERNAL_H */
