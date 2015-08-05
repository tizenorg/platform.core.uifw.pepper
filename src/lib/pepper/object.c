#include "pepper-internal.h"

static int
user_data_hash(const void *key, int key_length)
{
#if INTPTR_MAX == INT32_MAX
    return ((uint32_t)key) >> 2;
#elif INTPTR_MAX == INT64_MAX
    return ((uint64_t)key) >> 3;
#else
    #error "Not 32 or 64bit system"
#endif
}

static int
user_data_key_length(const void *key)
{
    return sizeof(key);
}

static int
user_data_key_compare(const void *key0, int key0_length, const void *key1, int key1_length)
{
    return (int)(key0 - key1);
}

pepper_object_t *
pepper_object_alloc(pepper_object_type_t type, size_t size)
{
    pepper_object_t *object = pepper_calloc(1, size);
    if (!object)
        return NULL;

    object->type = type;
    wl_signal_init(&object->destroy_signal);
    object->user_data_map = pepper_map_create(5, user_data_hash, user_data_key_length,
                                              user_data_key_compare);

    if (!object->user_data_map)
    {
        pepper_free(object);
        return NULL;
    }

    return object;
}

void
pepper_object_fini(pepper_object_t *object)
{
    wl_signal_emit(&object->destroy_signal, (void *)object);
    pepper_map_destroy(object->user_data_map);
}

PEPPER_API pepper_object_type_t
pepper_object_get_type(pepper_object_t *object)
{
    return object->type;
}

PEPPER_API void
pepper_object_set_user_data(pepper_object_t *object, const void *key, void *data,
                            pepper_free_func_t free_func)
{
    pepper_map_set(object->user_data_map, key, data, free_func);
}

PEPPER_API void *
pepper_object_get_user_data(pepper_object_t *object, const void *key)
{
    return pepper_map_get(object->user_data_map, key);
}

PEPPER_API void
pepper_object_add_destroy_listener(pepper_object_t *object, struct wl_listener *listener)
{
    wl_signal_add(&object->destroy_signal, listener);
}

static void
insert_listener(pepper_event_listener_t *listener)
{
    pepper_list_t   *l;

    PEPPER_LIST_FOR_EACH(&listener->object->event_listener_list, l)
    {
        pepper_event_listener_t *pos = l->item;

        if (listener->priority >= pos->priority)
        {
            pepper_list_insert(&pos->link, &listener->link);
            break;
        }
    }

    if (!listener->link.next)
        pepper_list_insert(listener->object->event_listener_list.prev, &listener->link);
}

PEPPER_API pepper_event_listener_t *
pepper_object_add_event_listener(pepper_object_t *object, uint32_t id,
                                 pepper_event_callback_t callback, int priority, void *data)
{
    pepper_event_listener_t *listener;
    pepper_list_t           *l;

    if (!callback)
        return NULL;

    listener = pepper_calloc(1, sizeof(pepper_event_listener_t));
    if (!listener)
        return NULL;

    listener->object    = object;
    listener->id        = id;
    listener->callback  = callback;
    listener->priority  = priority;
    listener->data      = data;

    insert_listener(listener);
    return listener;
}

PEPPER_API pepper_object_t *
pepper_event_listener_get_object(pepper_event_listener_t *listener)
{
    return listener->object;
}

PEPPER_API uint32_t
pepper_event_listener_get_id(pepper_event_listener_t *listener)
{
    return listener->id;
}

PEPPER_API pepper_event_callback_t
pepper_event_listener_get_callback(pepper_event_listener_t *listener)
{
    return listener->callback;
}

PEPPER_API int
pepper_event_listener_get_priority(pepper_event_listener_t *listener)
{
    return listener->priority;
}

PEPPER_API void *
pepper_event_listener_get_data(pepper_event_listener_t *listener)
{
    return listener->data;
}

PEPPER_API void
pepper_event_listener_set_priority(pepper_event_listener_t *listener, int priority)
{
    listener->priority = priority;
    pepper_list_remove(&listener->link, NULL);
    insert_listener(listener);
}

PEPPER_API void
pepper_event_listener_destroy(pepper_event_listener_t *listener)
{
    pepper_list_remove(&listener->link, NULL);
    pepper_free(listener);
}

void
pepper_object_signal_event(pepper_object_t *object, uint32_t id, void *info)
{
    pepper_event_listener_t *listener;
    pepper_list_t           *l;

    PEPPER_LIST_FOR_EACH(&object->event_listener_list, l)
    {
        listener = l->item;

        if (listener->id == id)
            listener->callback(listener, object, id, listener->data, info);
    }
}
