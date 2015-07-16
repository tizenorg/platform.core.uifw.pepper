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
pepper_object_alloc(size_t size)
{
    pepper_object_t *object = pepper_calloc(1, size);
    if (!object)
        return NULL;

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
