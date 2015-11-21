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

#include "pepper-internal.h"

static void
buffer_resource_destroy_handler(struct wl_listener *listener, void *data)
{
    pepper_buffer_t *buffer = pepper_container_of(listener, buffer, resource_destroy_listener);
    pepper_object_fini(&buffer->base);
    free(buffer);
}

pepper_buffer_t *
pepper_buffer_from_resource(struct wl_resource *resource)
{
    pepper_buffer_t     *buffer;
    struct wl_listener  *listener;

    listener = wl_resource_get_destroy_listener(resource, buffer_resource_destroy_handler);

    if (listener)
        return pepper_container_of(listener, buffer, resource_destroy_listener);

    buffer = (pepper_buffer_t *)pepper_object_alloc(PEPPER_OBJECT_BUFFER, sizeof(pepper_buffer_t));
    PEPPER_CHECK(buffer, return NULL, "pepper_object_alloc() failed.\n");

    buffer->resource = resource;
    buffer->resource_destroy_listener.notify = buffer_resource_destroy_handler;
    wl_resource_add_destroy_listener(resource, &buffer->resource_destroy_listener);

    return buffer;
}

PEPPER_API void
pepper_buffer_reference(pepper_buffer_t *buffer)
{
    PEPPER_ASSERT(buffer->ref_count >= 0);
    buffer->ref_count++;
}

PEPPER_API void
pepper_buffer_unreference(pepper_buffer_t *buffer)
{
    PEPPER_ASSERT(buffer->ref_count > 0);

    if (--buffer->ref_count == 0)
    {
        wl_resource_queue_event(buffer->resource, WL_BUFFER_RELEASE);
        pepper_object_emit_event(&buffer->base, PEPPER_EVENT_BUFFER_RELEASE, NULL);
    }
}

PEPPER_API struct wl_resource *
pepper_buffer_get_resource(pepper_buffer_t *buffer)
{
    return buffer->resource;
}
