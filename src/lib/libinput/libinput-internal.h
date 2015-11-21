/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

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
