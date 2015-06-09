#ifndef PEPPER_UTILS_H
#define PEPPER_UTILS_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#   define PEPPER_API __attribute__ ((visibility("default")))
#else
#   define PEPPER_API
#endif

#define pepper_container_of(ptr, type, member) ({               \
    const __typeof__( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})

typedef void (*pepper_free_func_t)(void *);

typedef unsigned int    pepper_bool_t;

#define PEPPER_FALSE    0
#define PEPPER_TRUE     1

typedef struct pepper_list  pepper_list_t;

#define PEPPER_LIST_FOR_EACH(head, pos)                     \
    for (pos = (head)->next;                                \
         pos != (head);                                     \
         pos = pos->next)

#define PEPPER_LIST_FOR_EACH_SAFE(head, pos, temp)          \
    for (pos = (head)->next, temp = pos->next;              \
         pos != (head);                                     \
         pos = temp, temp = pos->next)

#define PEPPER_LIST_FOR_EACH_REVERSE(head, pos)             \
    for (pos = (head)->prev;                                \
         pos != (head);                                     \
         pos = pos->prev)

#define PEPPER_LIST_FOR_EACH_REVERSE_SAFE(head, pos, temp)  \
    for (pos = (head)->prev, temp = pos->prev;              \
         pos != (head);                                     \
         pos = temp, temp = pos->prev)

struct pepper_list
{
    pepper_list_t  *prev;
    pepper_list_t  *next;
    void           *item;
};

static inline void
pepper_list_init(pepper_list_t *list)
{
    list->prev = list;
    list->next = list;
    list->item = NULL;
}

static inline void
pepper_list_insert(pepper_list_t *list, pepper_list_t *elm)
{
    elm->prev = list;
    elm->next = list->next;
    list->next = elm;
    elm->next->prev = elm;
}

static inline pepper_list_t *
pepper_list_insert_item(pepper_list_t *list, void *item)
{
    pepper_list_t *elm;

    elm = malloc(sizeof(pepper_list_t));
    if (!elm)
        return NULL;

    elm->item = item;
    pepper_list_insert(list, elm);
    return elm;
}

static inline void
pepper_list_remove(pepper_list_t *list, pepper_free_func_t free_func, pepper_bool_t free_list)
{
    list->prev->next = list->next;
    list->next->prev = list->prev;
    list->prev = NULL;
    list->next = NULL;

    if (free_func)
        free_func(list->item);

    if (free_list)
        free(list);
}

static inline void
pepper_list_remove_item(pepper_list_t *list, void *item,
                        pepper_free_func_t free_func, pepper_bool_t free_list)
{
    pepper_list_t *l;

    PEPPER_LIST_FOR_EACH(list, l)
    {
        if (l->item == item)
        {
            pepper_list_remove(l, free_func, free_list);
            return;
        }
    }
}

static inline void
pepper_list_clear(pepper_list_t *list, pepper_free_func_t free_func, pepper_bool_t free_list)
{
    pepper_list_t *l, *temp;

    PEPPER_LIST_FOR_EACH_SAFE(list, l, temp)
    {
        if (free_func)
            free_func(l->item);

        if (free_list)
            free(l);
    }

    pepper_list_init(list);
}

static inline pepper_bool_t
pepper_list_empty(const pepper_list_t *list)
{
    return list->next == list;
}

static inline void
pepper_list_insert_list(pepper_list_t *list, pepper_list_t *other)
{
    if (pepper_list_empty(other))
        return;

    other->next->prev = list;
    other->prev->next = list->next;
    list->next->prev = other->prev;
    list->next = other->next;
}

typedef struct pepper_map_entry pepper_map_entry_t;
typedef struct pepper_map       pepper_map_t;

typedef int (*pepper_hash_func_t)(const void *key, int key_length);
typedef int (*pepper_key_length_func_t)(const void *key);
typedef int (*pepper_key_compare_func_t)(const void *key0, int key0_length,
                                         const void *key1, int key1_length);

PEPPER_API pepper_map_t *
pepper_map_create(int                       bucket_bits,
                  pepper_hash_func_t        hash_func,
                  pepper_key_length_func_t  key_length_func,
                  pepper_key_compare_func_t key_compare_func);

PEPPER_API void
pepper_map_clear(pepper_map_t *map);

PEPPER_API void
pepper_map_destroy(pepper_map_t *map);

PEPPER_API void *
pepper_map_get(pepper_map_t *map, const void *key);

PEPPER_API void
pepper_map_set(pepper_map_t *map, const void *key, void *data, pepper_free_func_t free_func);

PEPPER_API int
pepper_create_anonymous_file(off_t size);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_UTILS_H */
