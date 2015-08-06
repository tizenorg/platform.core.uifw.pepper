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
    object->user_data_map = pepper_map_create(5, user_data_hash, user_data_key_length,
                                              user_data_key_compare);

    if (!object->user_data_map)
    {
        pepper_free(object);
        return NULL;
    }

    pepper_list_init(&object->event_listener_list);
    return object;
}

void
pepper_object_fini(pepper_object_t *object)
{
    pepper_object_emit_event(object, PEPPER_EVENT_OBJECT_DESTROY, NULL);
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

static void
insert_listener(pepper_object_t *object, pepper_event_listener_t *listener)
{
    pepper_list_t   *l;

    PEPPER_LIST_FOR_EACH(&object->event_listener_list, l)
    {
        pepper_event_listener_t *pos = l->item;

        if (listener->priority >= pos->priority)
        {
            pepper_list_insert(pos->link.prev, &listener->link);
            break;
        }
    }

    if (!listener->link.next)
        pepper_list_insert(object->event_listener_list.prev, &listener->link);
}

PEPPER_API void
pepper_event_listener_init(pepper_event_listener_t *listener)
{
    memset(listener, 0x00, sizeof(pepper_event_listener_t));
    listener->link.item = listener;
}

PEPPER_API void
pepper_object_add_event_listener(pepper_object_t *object,
                                 pepper_event_listener_t *listener,
                                 uint32_t id, int priority)
{
    listener->object    = object;
    listener->id        = id;
    listener->priority  = priority;

    insert_listener(object, listener);
}

PEPPER_API void
pepper_event_listener_remove(pepper_event_listener_t *listener)
{
    if (!listener->object)
        return;

    pepper_list_remove(&listener->link, NULL);
    listener->object = NULL;
}

PEPPER_API void
pepper_event_listener_set_priority(pepper_event_listener_t *listener, int priority)
{
    if (!listener->object)
        return;

    listener->priority = priority;
    pepper_list_remove(&listener->link, NULL);
    insert_listener(listener->object, listener);
}

PEPPER_API void
pepper_object_emit_event(pepper_object_t *object, uint32_t id, void *info)
{
    pepper_event_listener_t *listener;
    pepper_list_t           *l, *tmp;

    PEPPER_LIST_FOR_EACH_SAFE(&object->event_listener_list, l, tmp)
    {
        listener = l->item;

        if (listener->id == id)
            listener->callback(listener, object, id, info);
    }
}
