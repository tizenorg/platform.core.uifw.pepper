#ifndef PEPPER_UTILS_H
#define PEPPER_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

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

typedef struct pepper_list      pepper_list_t;
typedef struct pepper_matrix    pepper_matrix_t;

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

PEPPER_API int
pepper_log(const char* domain, int level, const char *format, ...);

struct pepper_matrix
{
    double      m[16];
    uint32_t    flags;
};

enum {
    PEPPER_MATRIX_TRANSLATE     = (1 << 0),
    PEPPER_MATRIX_SCALE         = (1 << 1),
    PEPPER_MATRIX_ROTATE        = (1 << 2),
    PEPPER_MATRIX_COMPLEX       = (1 << 3),
};

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
pepper_matrix_multiply(pepper_matrix_t *dst, const pepper_matrix_t *ma, const pepper_matrix_t *mb)
{
    pepper_matrix_t  tmp;
    double          *d = tmp.m;
    const double    *a = ma->m;
    const double    *b = mb->m;

    if (!ma->flags)
    {
        memcpy(dst, mb, sizeof(pepper_matrix_t));
        return;
    }

    if (!mb->flags)
    {
        memcpy(dst, ma, sizeof(pepper_matrix_t));
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
    memcpy(dst, &tmp, sizeof(pepper_matrix_t));
}

static inline void
pepper_matrix_init_identity(pepper_matrix_t *matrix)
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
pepper_matrix_init_translate(pepper_matrix_t *matrix, double x, double y, double z)
{
    pepper_matrix_init_identity(matrix);

    matrix->m[ 3] = x;
    matrix->m[ 7] = y;
    matrix->m[11] = z;

    matrix->flags |= PEPPER_MATRIX_TRANSLATE;
}

static inline void
pepper_matrix_translate(pepper_matrix_t *matrix, double x, double y, double z)
{
    matrix->m[ 3] = matrix->m[0] * x + matrix->m[1] * y + matrix->m[ 2] * z;
    matrix->m[ 7] = matrix->m[4] * x + matrix->m[5] * y + matrix->m[ 6] * z;
    matrix->m[11] = matrix->m[8] * x + matrix->m[9] * y + matrix->m[10] * z;

    matrix->flags |= PEPPER_MATRIX_TRANSLATE;
}

static inline void
pepper_matrix_init_scale(pepper_matrix_t *matrix, double x, double y, double z)
{
    pepper_matrix_init_identity(matrix);

    matrix->m[ 0] = x;
    matrix->m[ 5] = y;
    matrix->m[10] = z;

    matrix->flags |= PEPPER_MATRIX_SCALE;
}

static inline void
pepper_matrix_scale(pepper_matrix_t *matrix, double x, double y, double z)
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

    matrix->flags |= PEPPER_MATRIX_SCALE;
}

static inline void
pepper_matrix_init_rotate(pepper_matrix_t *matrix, double x, double y, double z, double angle)
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
        pepper_matrix_init_identity(matrix);
        return;
    }

    matrix->flags |= PEPPER_MATRIX_ROTATE;

    c = cos(angle);
    s = sin(angle);

    if (x == 0.0f && y == 0.0f)
    {
        pepper_matrix_init_identity(matrix);

        matrix->m[ 0] =  c;
        matrix->m[ 1] = -s;
        matrix->m[ 4] = -s;
        matrix->m[ 5] =  c;
    }
    else if (y == 0.0f && z == 0.0f)
    {
        pepper_matrix_init_identity(matrix);

        matrix->m[ 5] =  c;
        matrix->m[ 6] = -s;
        matrix->m[ 9] = -s;
        matrix->m[10] =  c;
    }
    else if (x == 0.0f && z == 0.0f)
    {
        pepper_matrix_init_identity(matrix);

        matrix->m[ 2] =  c;
        matrix->m[ 0] = -s;
        matrix->m[10] = -s;
        matrix->m[ 8] =  c;
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
        matrix->m[ 1] = x * yinvc - zs;
        matrix->m[ 2] = x * zinvc + ys;
        matrix->m[ 3] = 0.0f;

        matrix->m[ 4] = y * xinvc + zs;
        matrix->m[ 5] = c + y * yinvc;
        matrix->m[ 6] = y * zinvc - xs;
        matrix->m[ 7] = 0.0f;

        matrix->m[ 8] = z * xinvc - ys;
        matrix->m[ 9] = z * yinvc + xs;
        matrix->m[10] = c + z * zinvc;
        matrix->m[11] = 0.0f;

        matrix->m[12] = 0.0f;
        matrix->m[13] = 0.0f;
        matrix->m[14] = 0.0f;
        matrix->m[15] = 1.0f;
    }
}

static inline void
pepper_matrix_rotate(pepper_matrix_t *matrix, double x, double y, double z, double angle)
{
    pepper_matrix_t rotate;

    pepper_matrix_init_rotate(&rotate, x, y, z, angle);
    pepper_matrix_multiply(matrix, matrix, &rotate);
}

static inline void
pepper_matrix_copy(pepper_matrix_t *dst, const pepper_matrix_t *src)
{
    memcpy(dst, src, sizeof(pepper_matrix_t));
}

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_UTILS_H */
