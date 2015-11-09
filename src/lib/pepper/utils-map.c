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

#include "pepper-utils.h"
#include "pepper-utils.h"

struct pepper_map_entry
{
    const void         *key;
    void               *data;
    pepper_free_func_t  free_func;
    pepper_map_entry_t *next;
};

static inline int
get_bucket_index(pepper_map_t *map, const void *key)
{
    int                 key_length = 0;
    int                 hash;

    if (map->key_length_func)
        key_length = map->key_length_func(key);

    hash = map->hash_func(key, key_length);
    return hash & map->bucket_mask;
}

static inline pepper_map_entry_t **
get_bucket(pepper_map_t *map, const void *key)
{
    return &map->buckets[get_bucket_index(map, key)];
}

PEPPER_API void
pepper_map_init(pepper_map_t               *map,
                int                         bucket_bits,
                pepper_hash_func_t          hash_func,
                pepper_key_length_func_t    key_length_func,
                pepper_key_compare_func_t   key_compare_func,
                void                       *buckets)
{
    map->hash_func = hash_func;
    map->key_length_func = key_length_func;
    map->key_compare_func = key_compare_func;

    map->bucket_bits = bucket_bits;
    map->bucket_size = 1 << bucket_bits;
    map->bucket_mask = map->bucket_size - 1;

    map->buckets = buckets;
}

PEPPER_API void
pepper_map_fini(pepper_map_t *map)
{
    pepper_map_clear(map);
}

PEPPER_API pepper_map_t *
pepper_map_create(int                       bucket_bits,
                  pepper_hash_func_t        hash_func,
                  pepper_key_length_func_t  key_length_func,
                  pepper_key_compare_func_t key_compare_func)
{
    pepper_map_t   *map;
    int             bucket_size = 1 << bucket_bits;

    map = calloc(1, sizeof(pepper_map_t) + bucket_size * sizeof(pepper_map_entry_t *));
    PEPPER_CHECK(map, return NULL, "calloc() failed.\n");

    pepper_map_init(map, bucket_bits, hash_func, key_length_func, key_compare_func, map + 1);
    return map;
}

PEPPER_API void
pepper_map_destroy(pepper_map_t *map)
{
    pepper_map_fini(map);
    free(map->buckets);
    free(map);
}

PEPPER_API void
pepper_map_clear(pepper_map_t *map)
{
    int i;

    if (!map->buckets)
        return;

    for (i = 0; i < map->bucket_size; i++)
    {
        pepper_map_entry_t *curr = map->buckets[i];

        while (curr)
        {
            pepper_map_entry_t *next = curr->next;

            if (curr->free_func)
                curr->free_func(curr->data);

            free(curr);
            curr = next;
        }
    }

    memset(map->buckets, 0x00, map->bucket_size * sizeof(pepper_map_entry_t *));
}

PEPPER_API void *
pepper_map_get(pepper_map_t *map, const void *key)
{
    pepper_map_entry_t *curr = *get_bucket(map, key);

    while (curr)
    {
        int len0 = 0;
        int len1 = 0;

        if (map->key_length_func)
        {
            len0 = map->key_length_func(curr->key);
            len1 = map->key_length_func(key);
        }

        if (map->key_compare_func(curr->key, len0, key, len1) == 0)
            return curr->data;

        curr = curr->next;
    }

    return NULL;
}

PEPPER_API void
pepper_map_set(pepper_map_t *map, const void *key, void *data, pepper_free_func_t free_func)
{
    pepper_map_entry_t    **bucket = get_bucket(map, key);
    pepper_map_entry_t     *curr = *bucket;
    pepper_map_entry_t     *prev = NULL;

    /* Find existing entry for the key. */
    while (curr)
    {
        int len0 = 0;
        int len1 = 0;

        if (map->key_length_func)
        {
            len0 = map->key_length_func(curr->key);
            len1 = map->key_length_func(key);
        }

        if (map->key_compare_func(curr->key, len0, key, len1) == 0)
        {
            /* Free previous data. */
            if (curr->free_func)
                curr->free_func(curr->data);

            if (data)
            {
                /* Set new data. */
                curr->data = data;
                curr->free_func = free_func;
            }
            else
            {
                /* Delete entry. */
                if (prev)
                    prev->next = curr->next;
                else
                    *bucket = curr->next;

                free(curr);
            }

            return;
        }

        prev = curr;
        curr = curr->next;
    }

    if (data == NULL)
    {
        /* Nothing to delete. */
        return;
    }

    /* Allocate a new entry. */
    curr = malloc(sizeof(pepper_map_entry_t));
    PEPPER_CHECK(curr, return, "malloc() failed.\n");

    curr->key = key;
    curr->data = data;
    curr->free_func = free_func;

    /* Insert at the head of the bucket. */
    curr->next = *bucket;
    *bucket = curr;
}
