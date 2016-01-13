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

#define PEPPER_OBJECT_MAP_BUCKET_BITS   8

static pepper_id_allocator_t    id_allocator;
static pepper_map_t            *object_map = NULL;

pepper_object_t *
pepper_object_alloc(pepper_object_type_t type, size_t size)
{
    pepper_object_t *object = calloc(1, size);
    PEPPER_CHECK(object, return NULL, "calloc() failed.\n");
    pepper_object_init(object, type);
    return object;
}

void
pepper_object_init(pepper_object_t *object, pepper_object_type_t type)
{
    object->type = type;
    pepper_list_init(&object->event_listener_list);

#if INTPTR_MAX == INT32_MAX
    pepper_map_int32_init(&object->user_data_map, PEPPER_OBJECT_BUCKET_BITS, &object->buckets[0]);
#elif INTPTR_MAX == INT64_MAX
    pepper_map_int64_init(&object->user_data_map, PEPPER_OBJECT_BUCKET_BITS, &object->buckets[0]);
#else
    #error "Not 32 or 64bit system"
#endif

    if (!object_map)
    {
        pepper_id_allocator_init(&id_allocator);

        object_map = pepper_map_int32_create(PEPPER_OBJECT_MAP_BUCKET_BITS);
        PEPPER_CHECK(object_map, return, "pepper_map_int32_create() failed.\n");
    }

    object->id = pepper_id_allocator_alloc(&id_allocator);
}

void
pepper_object_fini(pepper_object_t *object)
{
    pepper_event_listener_t *listener, *tmp;

    pepper_object_emit_event(object, PEPPER_EVENT_OBJECT_DESTROY, NULL);
    pepper_map_fini(&object->user_data_map);

    pepper_list_for_each_safe(listener, tmp, &object->event_listener_list, link)
        pepper_event_listener_remove(listener);

    pepper_map_set(object_map, &object->id, NULL, NULL);
    pepper_id_allocator_free(&id_allocator, object->id);
}

/**
 * Get the type of the given object
 *
 * @param object    (none)
 *
 * @return type of the object
 */
PEPPER_API pepper_object_type_t
pepper_object_get_type(pepper_object_t *object)
{
    return object->type;
}

/**
 * Set user data to the given object with the given key
 *
 * @param object    (none)
 * @param key       (none)
 * @param data      (none)
 * @param free_func function to free the user data when the object is destroyed
 *
 * Only a single user data can be set for a key simultaneously. Be aware not to overwrite previous
 * user data by checking existing user data with #pepper_object_get_user_data() before write.
 *
 * @see pepper_object_get_user_data()
 */
PEPPER_API void
pepper_object_set_user_data(pepper_object_t *object, const void *key, void *data,
                            pepper_free_func_t free_func)
{
    pepper_map_set(&object->user_data_map, &key, data, free_func);
}

/**
 * Get the user data of the given object for the given key
 *
 * @param object    (none)
 * @param key       (none)
 *
 * @return the user data which has been set for the given key
 *
 * @see pepper_object_set_user_data()
 */
PEPPER_API void *
pepper_object_get_user_data(pepper_object_t *object, const void *key)
{
    return pepper_map_get(&object->user_data_map, &key);
}

static void
insert_listener(pepper_object_t *object, pepper_event_listener_t *listener)
{
    pepper_event_listener_t *pos;

    pepper_list_for_each(pos, &object->event_listener_list, link)
    {
        if (listener->priority >= pos->priority)
        {
            pepper_list_insert(pos->link.prev, &listener->link);
            break;
        }
    }

    if (!listener->link.next)
        pepper_list_insert(object->event_listener_list.prev, &listener->link);
}

/**
 * Add an event listener to the given object
 *
 * @param object    (none)
 * @param id        event id
 * @param priority  priority (higher priority called earlier)
 * @param callback  callback to be called when the event is emitted
 * @param data      data passed to the callback when the event is emitted
 *
 * @return added event listener object
 *
 * @see pepper_event_listener_remove()
 * @see pepper_event_listener_set_priority()
 */
PEPPER_API pepper_event_listener_t *
pepper_object_add_event_listener(pepper_object_t *object, uint32_t id, int priority,
                                 pepper_event_callback_t callback, void *data)
{
    pepper_event_listener_t *listener;

    PEPPER_CHECK(callback, return NULL, "callback must be given.\n");

    listener = calloc(1, sizeof(pepper_event_listener_t));
    PEPPER_CHECK(listener, return NULL, "calloc() failed.\n");

    listener->object    = object;
    listener->id        = id;
    listener->priority  = priority;
    listener->callback  = callback;
    listener->data      = data;

    insert_listener(object, listener);
    return listener;
}

/**
 * Remove an event listener from the belonging object
 *
 * @param listener  event listener
 *
 * @see pepper_object_add_event_listener()
 */
PEPPER_API void
pepper_event_listener_remove(pepper_event_listener_t *listener)
{
    pepper_list_remove(&listener->link);
    free(listener);
}

/**
 * Set priority of the given event listener
 *
 * @param listener  event listener
 * @param priority  priority (higher priority called earlier)
 */
PEPPER_API void
pepper_event_listener_set_priority(pepper_event_listener_t *listener, int priority)
{
    if (!listener->object)
        return;

    listener->priority = priority;
    pepper_list_remove(&listener->link);
    insert_listener(listener->object, listener);
}

/**
 * Emit an event to the given object
 *
 * @param object    (none)
 * @param id        event id
 * @param info      event info passed to the listeners
 *
 * Emitting an event to an object causes registered listeners are called. Higher priority listeners
 * will get called ealier than lower priority listeners. The type of the info is dependent on the
 * event type. Event info for the built-in events are defined in #pepper_built_in_events. Emitting
 * #PEPPER_EVENT_ALL has no effect. Some event ids are reserved by pepper which is defined in
 * #pepper_built_in_event.
 *
 * @see pepper_object_add_event_listener()
 * @see pepper_built_in_event
 */
PEPPER_API void
pepper_object_emit_event(pepper_object_t *object, uint32_t id, void *info)
{
    pepper_event_listener_t *listener, *tmp;

    PEPPER_CHECK(id != PEPPER_EVENT_ALL, return, "Cannot emit the PEPPER_EVENT_ALL event");

    pepper_list_for_each_safe(listener, tmp, &object->event_listener_list, link)
    {
        if (listener->id == PEPPER_EVENT_ALL || listener->id == id)
            listener->callback(listener, object, id, info, listener->data);
    }
}

/**
 * Get an unique 32bit integer ID of the given object
 *
 * @param object    (none)
 *
 * @return unique 32bit integer ID
 *
 * @see pepper_object_from_id()
 */
PEPPER_API uint32_t
pepper_object_get_id(pepper_object_t *object)
{
    return object->id;
}

/**
 * Get an object from the given ID
 *
 * @param id    (none)
 *
 * @return pointer to the #pepper_object_t if exist, NULL otherwise
 *
 * @see pepper_object_get_id()
 */
PEPPER_API pepper_object_t *
pepper_object_from_id(uint32_t id)
{
    return (pepper_object_t *)pepper_map_get(object_map, &id);
}
