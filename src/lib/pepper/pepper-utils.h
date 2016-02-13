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

#ifndef PEPPER_UTILS_H
#define PEPPER_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include <pixman.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#   define PEPPER_API __attribute__ ((visibility("default")))
#else
#   define PEPPER_API
#endif

#define PEPPER_MAX(a, b)    ((a) > (b) ? (a) : (b))
#define PEPPER_MIN(a, b)    ((a) < (b) ? (a) : (b))

#define pepper_container_of(ptr, sample, member)                                    \
	(__typeof__(sample))((char *)(ptr) - offsetof(__typeof__(*sample), member))

#define PEPPER_ARRAY_LENGTH(arr)    (sizeof(arr) / sizeof(arr)[0])

typedef void (*pepper_free_func_t)(void *);

typedef unsigned int    pepper_bool_t;

#define PEPPER_FALSE    0
#define PEPPER_TRUE     1

#define PEPPER_FORMAT(type, bpp, a, r, g, b)    \
    ((((type) & 0xff) << 24)    |               \
     (( (bpp) & 0xff) << 16)    |               \
     ((   (a) & 0x0f) << 12)    |               \
     ((   (r) & 0x0f) <<  8)    |               \
     ((   (g) & 0x0f) <<  4)    |               \
     ((   (b) & 0x0f) <<  0))

#define PEPPER_FORMAT_TYPE(format)  (((format) & 0xff000000) >> 24)
#define PEPPER_FORMAT_BPP(format)   (((format) & 0x00ff0000) >> 16)
#define PEPPER_FORMAT_A(format)     (((format) & 0x0000f000) >> 12)
#define PEPPER_FORMAT_R(format)     (((format) & 0x00000f00) >>  8)
#define PEPPER_FORMAT_G(format)     (((format) & 0x000000f0) >>  4)
#define PEPPER_FORMAT_B(format)     (((format) & 0x0000000f) >>  0)

typedef enum
{
    PEPPER_FORMAT_TYPE_UNKNOWN,
    PEPPER_FORMAT_TYPE_ARGB,
    PEPPER_FORMAT_TYPE_ABGR,
} pepper_format_type_t;

typedef enum
{
    PEPPER_FORMAT_UNKNOWN       = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_UNKNOWN,  0,  0,  0,  0,  0),

    PEPPER_FORMAT_ARGB8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    32,  8,  8,  8,  8),
    PEPPER_FORMAT_XRGB8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    32,  0,  8,  8,  8),
    PEPPER_FORMAT_RGB888        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    24,  0,  8,  8,  8),
    PEPPER_FORMAT_RGB565        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,    16,  0,  5,  6,  5),

    PEPPER_FORMAT_ABGR8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    32,  8,  8,  8,  8),
    PEPPER_FORMAT_XBGR8888      = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    32,  0,  8,  8,  8),
    PEPPER_FORMAT_BGR888        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    24,  0,  8,  8,  8),
    PEPPER_FORMAT_BGR565        = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ABGR,    16,  0,  5,  6,  5),

    PEPPER_FORMAT_ALPHA         = PEPPER_FORMAT(PEPPER_FORMAT_TYPE_ARGB,     8,  8,  0,  0,  0),
} pepper_format_t;

static inline pixman_format_code_t
get_pixman_format(pepper_format_t format)
{
    switch (format)
    {
    case PEPPER_FORMAT_ARGB8888:
        return PIXMAN_a8r8g8b8;
    case PEPPER_FORMAT_XRGB8888:
        return PIXMAN_x8r8g8b8;
    case PEPPER_FORMAT_RGB888:
        return PIXMAN_r8g8b8;
    case PEPPER_FORMAT_RGB565:
        return PIXMAN_r5g6b5;
    case PEPPER_FORMAT_ABGR8888:
        return PIXMAN_a8b8g8r8;
    case PEPPER_FORMAT_XBGR8888:
        return PIXMAN_x8b8g8r8;
    case PEPPER_FORMAT_BGR888:
        return PIXMAN_b8g8r8;
    case PEPPER_FORMAT_BGR565:
        return PIXMAN_b5g6r5;
    case PEPPER_FORMAT_ALPHA:
        return PIXMAN_a8;
    default:
        break;
    }

    return (pixman_format_code_t)0;
}

typedef struct pepper_list      pepper_list_t;

#define pepper_list_for_each_list(pos, head)                                \
    for (pos = (head)->next;                                                \
         pos != (head);                                                     \
         pos = pos->next)

#define pepper_list_for_each_list_safe(pos, tmp, head)                      \
    for (pos = (head)->next, tmp = pos->next;                               \
         pos != (head);                                                     \
         pos = tmp, tmp = pos->next)

#define pepper_list_for_each_list_reverse(pos, head)                        \
    for (pos = (head)->prev;                                                \
         pos != (head);                                                     \
         pos = pos->prev)

#define pepper_list_for_each_list_reverse_safe(pos, tmp, head)              \
    for (pos = (head)->prev, tmp = pos->prev;                               \
         pos != (head);                                                     \
         pos = tmp, tmp = pos->prev)

#define pepper_list_for_each(pos, head, member)                             \
    for (pos = pepper_container_of((head)->next, pos, member);              \
         &pos->member != (head);                                            \
         pos = pepper_container_of(pos->member.next, pos, member))

#define pepper_list_for_each_safe(pos, tmp, head, member)                   \
    for (pos = pepper_container_of((head)->next, pos, member),              \
         tmp = pepper_container_of((pos)->member.next, tmp, member);        \
         &pos->member != (head);                                            \
         pos = tmp,                                                         \
         tmp = pepper_container_of(pos->member.next, tmp, member))

#define pepper_list_for_each_reverse(pos, head, member)                     \
    for (pos = pepper_container_of((head)->prev, pos, member);              \
         &pos->member != (head);                                            \
         pos = pepper_container_of(pos->member.prev, pos, member))

#define pepper_list_for_each_reverse_safe(pos, tmp, head, member)           \
    for (pos = pepper_container_of((head)->prev, pos, member),              \
         tmp = pepper_container_of((pos)->member.prev, tmp, member);        \
         &pos->member != (head);                                            \
         pos = tmp,                                                         \
         tmp = pepper_container_of(pos->member.prev, tmp, member))

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

static inline void
pepper_list_remove(pepper_list_t *list)
{
    list->prev->next = list->next;
    list->next->prev = list->prev;
    list->prev = NULL;
    list->next = NULL;
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

/* Hash functions from Thomas Wang https://gist.github.com/badboy/6267743 */
static inline int
pepper_hash32(uint32_t key)
{
    key  = ~key + (key << 15);
    key ^= key >> 12;
    key += key << 2;
    key ^= key >> 4;
    key *= 2057;
    key ^= key >> 16;

    return key;
}

static inline int
pepper_hash64(uint64_t key)
{
    key  = ~key + (key << 18);
    key ^= key >> 31;
    key *= 21;
    key ^= key >> 11;
    key += key << 6;
    key ^= key >> 22;

    return (int)key;
}

typedef struct pepper_map_entry pepper_map_entry_t;
typedef struct pepper_map       pepper_map_t;
typedef union pepper_map_key   pepper_map_key_t;

typedef int (*pepper_hash_func_t)(const pepper_map_key_t key, int key_length);
typedef int (*pepper_key_length_func_t)(const pepper_map_key_t key);
typedef int (*pepper_key_compare_func_t)(const pepper_map_key_t key0, int key0_length,
                                         const pepper_map_key_t key1, int key1_length);
union pepper_map_key
{
    uint32_t    key32;
    uint64_t    key64;
    uintptr_t   keyPtr;
};

struct pepper_map
{
    pepper_hash_func_t          hash_func;
    pepper_key_length_func_t    key_length_func;
    pepper_key_compare_func_t   key_compare_func;

    int                         bucket_bits;
    int                         bucket_size;
    int                         bucket_mask;
    pepper_map_entry_t        **buckets;
};

PEPPER_API void
pepper_map_init(pepper_map_t               *map,
                int                         bucket_bits,
                pepper_hash_func_t          hash_func,
                pepper_key_length_func_t    key_length_func,
                pepper_key_compare_func_t   key_compare_func,
                void                       *buckets);

PEPPER_API void
pepper_map_int32_init(pepper_map_t *map, int bucket_bits, void *buckets);

PEPPER_API void
pepper_map_int64_init(pepper_map_t *map, int bucket_bits, void *buckets);

PEPPER_API void
pepper_map_pointer_init(pepper_map_t *map, int bucket_bits, void *buckets);

PEPPER_API void
pepper_map_fini(pepper_map_t *map);

PEPPER_API pepper_map_t *
pepper_map_create(int                       bucket_bits,
                  pepper_hash_func_t        hash_func,
                  pepper_key_length_func_t  key_length_func,
                  pepper_key_compare_func_t key_compare_func);

PEPPER_API pepper_map_t *
pepper_map_int32_create(int bucket_bits);

PEPPER_API pepper_map_t *
pepper_map_int64_create(int bucket_bits);

PEPPER_API pepper_map_t *
pepper_map_pointer_create(int bucket_bits);

PEPPER_API void
pepper_map_destroy(pepper_map_t *map);

PEPPER_API void
pepper_map_clear(pepper_map_t *map);

PEPPER_API void *
pepper_map_get(pepper_map_t *map, pepper_map_key_t key);

PEPPER_API void
pepper_map_set(pepper_map_t *map, pepper_map_key_t key, void *data, pepper_free_func_t free_func);

typedef struct pepper_id_allocator  pepper_id_allocator_t;

struct pepper_id_allocator
{
    uint32_t        next_id;
    uint32_t       *free_ids;
    int             free_id_count;
    int             free_id_size;
    int             free_id_head;
    int             free_id_tail;
};

PEPPER_API void
pepper_id_allocator_init(pepper_id_allocator_t *allocator);

PEPPER_API void
pepper_id_allocator_fini(pepper_id_allocator_t *allocator);

PEPPER_API uint32_t
pepper_id_allocator_alloc(pepper_id_allocator_t *allocator);

PEPPER_API void
pepper_id_allocator_free(pepper_id_allocator_t *allocator, uint32_t id);

PEPPER_API int
pepper_create_anonymous_file(off_t size);

PEPPER_API int
pepper_log(const char* domain, int level, const char *format, ...);

#define PEPPER_ERROR(fmt, ...)                                                          \
    do {                                                                                \
        pepper_log("ERROR", 0, "%s:%s: "fmt, __FILE__, __FUNCTION__, ##__VA_ARGS__);	\
    } while (0)

#define PEPPER_TRACE(fmt, ...)                                                          \
    do {                                                                                \
        pepper_log("DEBUG", 0, fmt, ##__VA_ARGS__);                                     \
    } while (0)

#define PEPPER_CHECK(exp, action, fmt, ...)                                             \
    do {                                                                                \
        if (!(exp))                                                                     \
        {                                                                               \
            PEPPER_ERROR(fmt, ##__VA_ARGS__);                                           \
            action;                                                                     \
        }                                                                               \
    } while (0)

PEPPER_API void
pepper_assert(pepper_bool_t exp);

#define PEPPER_ASSERT(exp)  assert(exp)

typedef struct pepper_mat4  pepper_mat4_t;
typedef struct pepper_vec4  pepper_vec4_t;
typedef struct pepper_vec3  pepper_vec3_t;
typedef struct pepper_vec2  pepper_vec2_t;

struct pepper_mat4
{
    double      m[16];
    uint32_t    flags;
};

struct pepper_vec2
{
    double      x, y;
};

struct pepper_vec3
{
    double      x, y, z;
};

struct pepper_vec4
{
    double      x, y, z, w;
};

enum {
    PEPPER_MATRIX_TRANSLATE     = (1 << 0),
    PEPPER_MATRIX_SCALE         = (1 << 1),
    PEPPER_MATRIX_ROTATE        = (1 << 2),
    PEPPER_MATRIX_COMPLEX       = (1 << 3),
};

static inline pepper_bool_t
pepper_mat4_is_translation(const pepper_mat4_t *matrix)
{
    return matrix->flags == PEPPER_MATRIX_TRANSLATE || matrix->flags == 0;
}

static inline double
pepper_reciprocal_sqrt(double x)
{
   union {
        float f;
        long  i;
   } u;

   u.f = x;
   u.i = 0x5f3759df - (u.i >> 1);
   return (double)(u.f * (1.5f - u.f * u.f * x * 0.5f));
}

static inline void
pepper_mat4_multiply(pepper_mat4_t *dst, const pepper_mat4_t *ma, const pepper_mat4_t *mb)
{
    pepper_mat4_t  tmp;
    double          *d = tmp.m;
    const double    *a = ma->m;
    const double    *b = mb->m;

    if (!ma->flags)
    {
        memcpy(dst, mb, sizeof(pepper_mat4_t));
        return;
    }

    if (!mb->flags)
    {
        memcpy(dst, ma, sizeof(pepper_mat4_t));
        return;
    }

    d[ 0] = a[ 0] * b[ 0] + a[ 4] * b[ 1] + a[ 8] * b[ 2] + a[12] * b [3];
    d[ 4] = a[ 0] * b[ 4] + a[ 4] * b[ 5] + a[ 8] * b[ 6] + a[12] * b [7];
    d[ 8] = a[ 0] * b[ 8] + a[ 4] * b[ 9] + a[ 8] * b[10] + a[12] * b[11];
    d[12] = a[ 0] * b[12] + a[ 4] * b[13] + a[ 8] * b[14] + a[12] * b[15];

    d[ 1] = a[ 1] * b[ 0] + a[ 5] * b[ 1] + a[ 9] * b[ 2] + a[13] * b [3];
    d[ 5] = a[ 1] * b[ 4] + a[ 5] * b[ 5] + a[ 9] * b[ 6] + a[13] * b [7];
    d[ 9] = a[ 1] * b[ 8] + a[ 5] * b[ 9] + a[ 9] * b[10] + a[13] * b[11];
    d[13] = a[ 1] * b[12] + a[ 5] * b[13] + a[ 9] * b[14] + a[13] * b[15];

    d[ 2] = a[ 2] * b[ 0] + a[ 6] * b[ 1] + a[10] * b[ 2] + a[14] * b [3];
    d[ 6] = a[ 2] * b[ 4] + a[ 6] * b[ 5] + a[10] * b[ 6] + a[14] * b [7];
    d[10] = a[ 2] * b[ 8] + a[ 6] * b[ 9] + a[10] * b[10] + a[14] * b[11];
    d[14] = a[ 2] * b[12] + a[ 6] * b[13] + a[10] * b[14] + a[14] * b[15];

    d[ 3] = a[ 3] * b[ 0] + a[ 7] * b[ 1] + a[11] * b[ 2] + a[15] * b [3];
    d[ 7] = a[ 3] * b[ 4] + a[ 7] * b[ 5] + a[11] * b[ 6] + a[15] * b [7];
    d[11] = a[ 3] * b[ 8] + a[ 7] * b[ 9] + a[11] * b[10] + a[15] * b[11];
    d[15] = a[ 3] * b[12] + a[ 7] * b[13] + a[11] * b[14] + a[15] * b[15];

    tmp.flags = ma->flags | mb->flags;
    memcpy(dst, &tmp, sizeof(pepper_mat4_t));
}

static inline void
pepper_mat4_init_identity(pepper_mat4_t *matrix)
{
    matrix->m[ 0] = 1.0f;
    matrix->m[ 1] = 0.0f;
    matrix->m[ 2] = 0.0f;
    matrix->m[ 3] = 0.0f;

    matrix->m[ 4] = 0.0f;
    matrix->m[ 5] = 1.0f;
    matrix->m[ 6] = 0.0f;
    matrix->m[ 7] = 0.0f;

    matrix->m[ 8] = 0.0f;
    matrix->m[ 9] = 0.0f;
    matrix->m[10] = 1.0f;
    matrix->m[11] = 0.0f;

    matrix->m[12] = 0.0f;
    matrix->m[13] = 0.0f;
    matrix->m[14] = 0.0f;
    matrix->m[15] = 1.0f;

    matrix->flags = 0;
}

static inline void
pepper_mat4_init_translate(pepper_mat4_t *matrix, double x, double y, double z)
{
    pepper_mat4_init_identity(matrix);

    matrix->m[12] = x;
    matrix->m[13] = y;
    matrix->m[14] = z;

    matrix->flags |= PEPPER_MATRIX_TRANSLATE;
}

static inline void
pepper_mat4_translate(pepper_mat4_t *matrix, double x, double y, double z)
{
    matrix->m[ 0] += matrix->m[ 3] * x;
    matrix->m[ 1] += matrix->m[ 3] * y;
    matrix->m[ 2] += matrix->m[ 3] * z;
    matrix->m[ 4] += matrix->m[ 7] * x;
    matrix->m[ 5] += matrix->m[ 7] * y;
    matrix->m[ 6] += matrix->m[ 7] * z;
    matrix->m[ 8] += matrix->m[11] * x;
    matrix->m[ 9] += matrix->m[11] * y;
    matrix->m[10] += matrix->m[11] * z;
    matrix->m[12] += matrix->m[15] * x;
    matrix->m[13] += matrix->m[15] * y;
    matrix->m[14] += matrix->m[15] * z;

    matrix->flags |= PEPPER_MATRIX_TRANSLATE;
}

static inline void
pepper_mat4_init_scale(pepper_mat4_t *matrix, double x, double y, double z)
{
    pepper_mat4_init_identity(matrix);

    matrix->m[ 0] = x;
    matrix->m[ 5] = y;
    matrix->m[10] = z;

    matrix->flags |= PEPPER_MATRIX_SCALE;
}

static inline void
pepper_mat4_scale(pepper_mat4_t *matrix, double x, double y, double z)
{
    matrix->m[ 0] *= x;
    matrix->m[ 1] *= y;
    matrix->m[ 2] *= z;

    matrix->m[ 4] *= x;
    matrix->m[ 5] *= y;
    matrix->m[ 6] *= z;

    matrix->m[ 8] *= x;
    matrix->m[ 9] *= y;
    matrix->m[10] *= z;

    matrix->m[12] *= x;
    matrix->m[13] *= y;
    matrix->m[14] *= z;

    matrix->flags |= PEPPER_MATRIX_SCALE;
}

static inline void
pepper_mat4_init_rotate(pepper_mat4_t *matrix, double x, double y, double z, double angle)
{
    double c;
    double s;
    double invlen;
    double xs;
    double ys;
    double zs;
    double invc;
    double xinvc;
    double yinvc;
    double zinvc;

    if (angle == 0.0f || (z == 0.0f && y == 0.0f && z == 0.0f))
    {
        pepper_mat4_init_identity(matrix);
        return;
    }

    c = cos(angle);
    s = sin(angle);

    if (x == 0.0f && y == 0.0f)
    {
        pepper_mat4_init_identity(matrix);

        matrix->m[ 0] =  c;
        matrix->m[ 1] =  s;
        matrix->m[ 4] = -s;
        matrix->m[ 5] =  c;
    }
    else if (y == 0.0f && z == 0.0f)
    {
        pepper_mat4_init_identity(matrix);

        matrix->m[ 5] =  c;
        matrix->m[ 6] =  s;
        matrix->m[ 9] = -s;
        matrix->m[10] =  c;
    }
    else if (x == 0.0f && z == 0.0f)
    {
        pepper_mat4_init_identity(matrix);

        matrix->m[ 0] =  c;
        matrix->m[ 2] = -s;
        matrix->m[ 8] =  s;
        matrix->m[10] =  c;
    }
    else
    {
        invlen = pepper_reciprocal_sqrt(x * x + y * y + z * z);

        x *= invlen;
        y *= invlen;
        z *= invlen;

        xs = x * s;
        ys = y * s;
        zs = z * s;
        invc = 1 - c;
        xinvc = x * invc;
        yinvc = y * invc;
        zinvc = z * invc;

        matrix->m[ 0] = c + x * xinvc;
        matrix->m[ 4] = x * yinvc - zs;
        matrix->m[ 8] = x * zinvc + ys;
        matrix->m[12] = 0.0f;

        matrix->m[ 1] = y * xinvc + zs;
        matrix->m[ 5] = c + y * yinvc;
        matrix->m[ 9] = y * zinvc - xs;
        matrix->m[13] = 0.0f;

        matrix->m[ 2] = z * xinvc - ys;
        matrix->m[ 6] = z * yinvc + xs;
        matrix->m[10] = c + z * zinvc;
        matrix->m[14] = 0.0f;

        matrix->m[ 3] = 0.0f;
        matrix->m[ 7] = 0.0f;
        matrix->m[11] = 0.0f;
        matrix->m[15] = 1.0f;
    }

    matrix->flags |= PEPPER_MATRIX_ROTATE;
}

static inline void
pepper_mat4_rotate(pepper_mat4_t *matrix, double x, double y, double z, double angle)
{
    pepper_mat4_t rotate;

    pepper_mat4_init_rotate(&rotate, x, y, z, angle);
    pepper_mat4_multiply(matrix, &rotate, matrix);
}

static inline void
pepper_mat4_copy(pepper_mat4_t *dst, const pepper_mat4_t *src)
{
    memcpy(dst, src, sizeof(pepper_mat4_t));
}

static inline void
pepper_mat4_inverse(pepper_mat4_t *dst, const pepper_mat4_t *src)
{
    pepper_mat4_t   tmp;
    double         *d = &tmp.m[0];
    const double   *m = &src->m[0];
    double          det;

    if (!(src->flags & PEPPER_MATRIX_COMPLEX) &&
        !(src->flags & PEPPER_MATRIX_ROTATE))
    {
        pepper_mat4_copy(dst, src);

        dst->m[12] = -m[12] / m[ 0];
        dst->m[13] = -m[13] / m[ 5];
        dst->m[14] = -m[14] / m[10];

        dst->m[ 0] = 1.0 / m[ 0];
        dst->m[ 5] = 1.0 / m[ 5];
        dst->m[10] = 1.0 / m[10];

        return;
    }

    d[ 0] =  m[ 5] * m[10] * m[15] -
             m[ 5] * m[11] * m[14] -
             m[ 9] * m[ 6] * m[15] +
             m[ 9] * m[ 7] * m[14] +
             m[13] * m[ 6] * m[11] -
             m[13] * m[ 7] * m[10];

    d[ 4] = -m[ 4] * m[10] * m[15] +
             m[ 4] * m[11] * m[14] +
             m[ 8] * m[ 6] * m[15] -
             m[ 8] * m[ 7] * m[14] -
             m[12] * m[ 6] * m[11] +
             m[12] * m[ 7] * m[10];

    d[ 8] =  m[ 4] * m[ 9] * m[15] -
             m[ 4] * m[11] * m[13] -
             m[ 8] * m[ 5] * m[15] +
             m[ 8] * m[ 7] * m[13] +
             m[12] * m[ 5] * m[11] -
             m[12] * m[ 7] * m[ 9];

    d[12] = -m[ 4] * m[ 9] * m[14] +
             m[ 4] * m[10] * m[13] +
             m[ 8] * m[ 5] * m[14] -
             m[ 8] * m[ 6] * m[13] -
             m[12] * m[ 5] * m[10] +
             m[12] * m[ 6] * m[ 9];

    d[ 1] = -m[ 1] * m[10] * m[15] +
             m[ 1] * m[11] * m[14] +
             m[ 9] * m[ 2] * m[15] -
             m[ 9] * m[ 3] * m[14] -
             m[13] * m[ 2] * m[11] +
             m[13] * m[ 3] * m[10];

    d[ 5] =  m[ 0] * m[10] * m[15] -
             m[ 0] * m[11] * m[14] -
             m[ 8] * m[ 2] * m[15] +
             m[ 8] * m[ 3] * m[14] +
             m[12] * m[ 2] * m[11] -
             m[12] * m[ 3] * m[10];

    d[ 9] = -m[ 0] * m[ 9] * m[15] +
             m[ 0] * m[11] * m[13] +
             m[ 8] * m[ 1] * m[15] -
             m[ 8] * m[ 3] * m[13] -
             m[12] * m[ 1] * m[11] +
             m[12] * m[ 3] * m[ 9];

    d[13] =  m[ 0] * m[ 9] * m[14] -
             m[ 0] * m[10] * m[13] -
             m[ 8] * m[ 1] * m[14] +
             m[ 8] * m[ 2] * m[13] +
             m[12] * m[ 1] * m[10] -
             m[12] * m[ 2] * m[ 9];

    d[ 2] =  m[ 1] * m[ 6] * m[15] -
             m[ 1] * m[ 7] * m[14] -
             m[ 5] * m[ 2] * m[15] +
             m[ 5] * m[ 3] * m[14] +
             m[13] * m[ 2] * m[ 7] -
             m[13] * m[ 3] * m[ 6];

    d[ 6] = -m[ 0] * m[ 6] * m[15] +
             m[ 0] * m[ 7] * m[14] +
             m[ 4] * m[ 2] * m[15] -
             m[ 4] * m[ 3] * m[14] -
             m[12] * m[ 2] * m[ 7] +
             m[12] * m[ 3] * m[ 6];

    d[10] =  m[ 0] * m[ 5] * m[15] -
             m[ 0] * m[ 7] * m[13] -
             m[ 4] * m[ 1] * m[15] +
             m[ 4] * m[ 3] * m[13] +
             m[12] * m[ 1] * m[ 7] -
             m[12] * m[ 3] * m[ 5];

    d[14] = -m[ 0] * m[ 5] * m[14] +
             m[ 0] * m[ 6] * m[13] +
             m[ 4] * m[ 1] * m[14] -
             m[ 4] * m[ 2] * m[13] -
             m[12] * m[ 1] * m[ 6] +
             m[12] * m[ 2] * m[ 5];

    d[ 3] = -m[ 1] * m[ 6] * m[11] +
             m[ 1] * m[ 7] * m[10] +
             m[ 5] * m[ 2] * m[11] -
             m[ 5] * m[ 3] * m[10] -
             m[ 9] * m[ 2] * m[ 7] +
             m[ 9] * m[ 3] * m[ 6];

    d[ 7] =  m[ 0] * m[ 6] * m[11] -
             m[ 0] * m[ 7] * m[10] -
             m[ 4] * m[ 2] * m[11] +
             m[ 4] * m[ 3] * m[10] +
             m[ 8] * m[ 2] * m[ 7] -
             m[ 8] * m[ 3] * m[ 6];

    d[11] = -m[ 0] * m[ 5] * m[11] +
             m[ 0] * m[ 7] * m[ 9] +
             m[ 4] * m[ 1] * m[11] -
             m[ 4] * m[ 3] * m[ 9] -
             m[ 8] * m[ 1] * m[ 7] +
             m[ 8] * m[ 3] * m[ 5];

    d[15] =  m[ 0] * m[ 5] * m[10] -
             m[ 0] * m[ 6] * m[ 9] -
             m[ 4] * m[ 1] * m[10] +
             m[ 4] * m[ 2] * m[ 9] +
             m[ 8] * m[ 1] * m[ 6] -
             m[ 8] * m[ 2] * m[ 5];

    det = m[0] * d[0] + m[1] * d[4] + m[2] * d[8] + m[3] * d[12];

    if (det == 0.0)
    {
        PEPPER_ERROR("Matrix is singular. Unable to get inverse matrix.\n");
        return;
    }

    det = 1.0 / det;

    d[ 0] *= det;
    d[ 1] *= det;
    d[ 2] *= det;
    d[ 3] *= det;
    d[ 4] *= det;
    d[ 5] *= det;
    d[ 6] *= det;
    d[ 7] *= det;
    d[ 8] *= det;
    d[ 9] *= det;
    d[10] *= det;
    d[11] *= det;
    d[12] *= det;
    d[13] *= det;
    d[14] *= det;
    d[15] *= det;

    pepper_mat4_copy(dst, &tmp);
    dst->flags = src->flags;
}

static inline void
pepper_mat4_transform_vec2(const pepper_mat4_t *matrix, pepper_vec2_t *v)
{
    double x, y;
    const double *m = &matrix->m[0];

    x = m[0] * v->x + m[4] * v->y + m[12];
    y = m[1] * v->x + m[5] * v->y + m[13];

    v->x = x;
    v->y = y;
}

static inline void
pepper_mat4_transform_vec3(const pepper_mat4_t *matrix, pepper_vec3_t *v)
{
    double x, y, z;
    const double *m = &matrix->m[0];

    x = m[0] * v->x + m[4] * v->y + m[ 8] * v->z + m[12];
    y = m[1] * v->x + m[5] * v->y + m[ 9] * v->z + m[13];
    z = m[2] * v->x + m[6] * v->y + m[10] * v->z + m[14];

    v->x = x;
    v->y = y;
    v->z = z;
}

static inline void
pepper_mat4_transform_vec4(const pepper_mat4_t *matrix, pepper_vec4_t *v)
{
    double x, y, z, w;
    const double *m = &matrix->m[0];

    x = m[0] * v->x + m[4] * v->y + m[ 8] * v->z + m[12] * v->w;
    y = m[1] * v->x + m[5] * v->y + m[ 9] * v->z + m[13] * v->w;
    z = m[2] * v->x + m[6] * v->y + m[10] * v->z + m[14] * v->w;
    w = m[3] * v->x + m[7] * v->y + m[11] * v->z + m[15] * v->w;

    v->x = x;
    v->y = y;
    v->z = z;
    v->w = w;
}

/* Virtual terminal */
PEPPER_API pepper_bool_t
pepper_virtual_terminal_setup(int tty);

PEPPER_API void
pepper_virtual_terminal_restore(void);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_UTILS_H */
