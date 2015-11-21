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

#ifndef PEPPER_INPUT_BACKEND_H
#define PEPPER_INPUT_BACKEND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_input_device_backend      pepper_input_device_backend_t;

struct pepper_input_device_backend
{
    const char *    (*get_property)(void *device, const char *key);
};

PEPPER_API pepper_input_device_t *
pepper_input_device_create(pepper_compositor_t *compositor, uint32_t caps,
                           const pepper_input_device_backend_t *backend, void *data);

PEPPER_API void
pepper_input_device_destroy(pepper_input_device_t *device);

PEPPER_API uint32_t
pepper_input_device_get_caps(pepper_input_device_t *device);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_INPUT_BACKEND_H */
