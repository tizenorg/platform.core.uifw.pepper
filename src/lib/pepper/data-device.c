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

#include <unistd.h>
#include "pepper-internal.h"

static void
data_offer_accept(struct wl_client      *client,
                  struct wl_resource    *resource,
                  uint32_t               serial,
                  const char            *mime_type)
{
    pepper_data_offer_t *offer = wl_resource_get_user_data(resource);

    if (offer->source)
        wl_data_source_send_target(offer->source->resource, mime_type);
}

static void
data_offer_receive(struct wl_client     *client,
                   struct wl_resource   *resource,
                   const char           *mime_type,
                   int32_t               fd)
{
    pepper_data_offer_t *offer = wl_resource_get_user_data(resource);

    if (offer->source)
        wl_data_source_send_send(offer->source->resource, mime_type, fd);

    close(fd);
}

static void
data_offer_destroy(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_data_offer_interface data_offer_interface =
{
    data_offer_accept,
    data_offer_receive,
    data_offer_destroy
};

static void destroy_data_offer(struct wl_resource *resource)
{
    pepper_data_offer_t *offer = wl_resource_get_user_data(resource);

    if (offer->source)
        wl_list_remove(&offer->source_destroy_listener.link);

    free(offer);
}

static void
destroy_offer_data_source(struct wl_listener *listener, void *data)
{
    pepper_data_offer_t *offer = pepper_container_of(listener, offer, source_destroy_listener);

    offer->source = NULL;
}

/* FIXME: */
static pepper_data_offer_t *
pepper_data_source_send_offer(pepper_data_source_t *source, struct wl_resource *resource)
{
    pepper_data_offer_t *offer;
    char **p;

    offer = calloc(1, sizeof(pepper_data_offer_t));
    if (!offer)
    {
        wl_resource_post_no_memory(resource);
        return NULL;
    }

    offer->resource = wl_resource_create(wl_resource_get_client(resource),
                                         &wl_data_offer_interface, 1, 0);
    if (!offer->resource)
    {
        free(offer);
        wl_resource_post_no_memory(resource);
        return NULL;
    }

    wl_resource_set_implementation(offer->resource, &data_offer_interface,
                                   offer, destroy_data_offer);

    offer->source = source;
    offer->source_destroy_listener.notify = destroy_offer_data_source;
    wl_signal_add(&source->destroy_signal, &offer->source_destroy_listener);

    wl_data_device_send_data_offer(resource, offer->resource);

    wl_array_for_each(p, &source->mime_types)
        wl_data_offer_send_offer(offer->resource, *p);

    return offer;
}


static void
data_device_start_drag(struct wl_client     *client,
                       struct wl_resource   *resource,
                       struct wl_resource   *source_resource,
                       struct wl_resource   *origin_resource,
                       struct wl_resource   *icon_resource,
                       uint32_t              serial)
{
    /* TODO */
    pepper_data_source_t *source = wl_resource_get_user_data(source_resource);

    PEPPER_ERROR("TODO:\n");

    pepper_data_source_send_offer(source, resource);
}

static void
data_device_set_selection(struct wl_client   *client,
                          struct wl_resource *resource,
                          struct wl_resource *source_resource,
                          uint32_t            serial)
{
    pepper_data_source_t *source = wl_resource_get_user_data(source_resource);
    PEPPER_ERROR("TODO:\n");
    pepper_data_source_send_offer(source, resource);

}

static void
data_device_release(struct wl_client *client, struct wl_resource *resource)
{
    PEPPER_ERROR("TODO:\n");
    wl_resource_destroy(resource);
}

static const struct wl_data_device_interface data_device_interface =
{
    data_device_start_drag,
    data_device_set_selection,
    data_device_release
};

static void
data_source_offer(struct wl_client   *client,
                  struct wl_resource *resource,
                  const char         *type)
{
    pepper_data_source_t *source = wl_resource_get_user_data(resource);
    char **p;

    p = wl_array_add(&source->mime_types, sizeof(char*));

    if (p)
        *p = strdup(type);

    if (!p || !*p)
        wl_resource_post_no_memory(resource);
}

static void
data_source_destroy(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static struct wl_data_source_interface data_source_interface =
{
    data_source_offer,
    data_source_destroy
};

static void
destroy_data_source(struct wl_resource *resource)
{
    pepper_data_source_t *source = wl_resource_get_user_data(resource);
    char **p;

    wl_signal_emit(&source->destroy_signal, source);

    wl_array_for_each(p, &source->mime_types)
        free(*p);

    wl_array_release(&source->mime_types);

    free(source);
}

static void
create_data_source(struct wl_client   *client,
                   struct wl_resource *resource,
                   uint32_t            id)
{
    pepper_data_source_t    *source;

    source = calloc(1, sizeof(pepper_data_source_t));
    if (!source)
    {
        wl_resource_post_no_memory(resource);
        return;
    }

    wl_signal_init(&source->destroy_signal);
    wl_array_init(&source->mime_types);

    source->resource = wl_resource_create(client, &wl_data_source_interface, 1, id);
    wl_resource_set_implementation(source->resource,
                                   &data_source_interface,
                                   source,
                                   destroy_data_source);
}

static void
destroy_data_device(struct wl_resource *resource)
{
    pepper_data_device_t *device = wl_resource_get_user_data(resource);

    /* remove item from seat->data_device_list */
    /* wl_list_remove(wl_resource_get_link(device->resource)); */

    free(device);
}

/*
 * wl_data_device_manager::get_data_device - create a new data device
 *
 * id
 *     id for the new wl_data_device
 * seat
 *     wl_seat
 *
 * Create a new data device for a given seat.
 */

static void
get_data_device(struct wl_client    *client,
                struct wl_resource  *manager_resource,
                uint32_t             id,
                struct wl_resource  *seat_resource)
{
    pepper_data_device_t    *data_device;
    pepper_seat_t           *seat;

    data_device = calloc(1, sizeof(pepper_data_device_t));
    if (!data_device)
    {
        wl_resource_post_no_memory(manager_resource);
        return;
    }

    data_device->resource = wl_resource_create(client,
                                               &wl_data_device_interface,
                                               wl_resource_get_version(manager_resource),
                                               id);
    if (!data_device->resource)
    {
        free(data_device);
        wl_resource_post_no_memory(manager_resource);
        return;
    }

    seat = wl_resource_get_user_data(seat_resource);

    /* set seat */
    data_device->seat = seat;

    /* TODO:
    wl_list_insert(&seat->data_device_list,
                   wl_resource_get_link(data_device->resource));
    */

    wl_resource_set_implementation(data_device->resource, &data_device_interface,
                                   data_device, destroy_data_device);
}

static const struct wl_data_device_manager_interface manager_interface =
{
    create_data_source,
    get_data_device
};

static void
data_device_manager_bind(struct wl_client   *client,
                         void               *data,
                         uint32_t            version,
                         uint32_t            id)
{
    struct wl_resource *resource;

    resource = wl_resource_create(client, &wl_data_device_manager_interface, version, id);
    if (!resource)
    {
        wl_client_post_no_memory(client);
        return;
    }

    wl_resource_set_implementation(resource, &manager_interface, NULL, NULL);
}

pepper_bool_t
pepper_data_device_manager_init(struct wl_display *display)
{
    if( wl_global_create(display, &wl_data_device_manager_interface,
                         2, NULL, data_device_manager_bind) == NULL )
    {
        return PEPPER_FALSE;
    }

    return PEPPER_TRUE;
}
