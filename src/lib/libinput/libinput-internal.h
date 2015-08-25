#ifndef LIBINPUT_INTERNAL_H
#define LIBINPUT_INTERNAL_H

#include "pepper-libinput.h"
#include <pepper-input-backend.h>

typedef struct li_device            li_device_t;
typedef struct li_device_property   li_device_property_t;

struct pepper_libinput
{
    pepper_compositor_t        *compositor;
    struct udev                *udev;
    struct libinput            *libinput;
    struct wl_event_source     *libinput_event_source;
    int                         libinput_fd;

    pepper_list_t               device_list;
};

struct li_device
{
    pepper_libinput_t          *input;
    pepper_input_device_t      *base;

    uint32_t                    caps;

    pepper_list_t               property_list;
    pepper_list_t               link;
};

struct li_device_property   /* FIXME */
{
    char                       *key;
    char                       *data;
    pepper_list_t               link;
};

#endif /* LIBINPUT_INTERNAL_H */
