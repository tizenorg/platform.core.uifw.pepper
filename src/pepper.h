#ifndef PEPPER_H
#define PEPPER_H

#define WL_HIDE_DEPRECATED
#include <wayland-server.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ >= 4
#   define PEPPER_API __attribute__ ((visibility("default")))
#else
#   define PEPPER_API
#endif

#define PEPPER_FALSE    0
#define PEPPER_TRUE     1

typedef unsigned int                    pepper_bool_t;
typedef struct pepper_compositor        pepper_compositor_t;

typedef struct pepper_output_geometry   pepper_output_geometry_t;
typedef struct pepper_output_mode       pepper_output_mode_t;
typedef struct pepper_output            pepper_output_t;
typedef struct pepper_output_interface  pepper_output_interface_t;

/* Compositor functions. */
PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name);

PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor);

PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor);

PEPPER_API void
pepper_compositor_set_user_data(pepper_compositor_t *compositor, uint32_t key, void *data);

PEPPER_API void *
pepper_compositor_get_user_data(pepper_compositor_t *compositor, uint32_t key);

/* Output. */
struct pepper_output_geometry
{
    int32_t     x;
    int32_t     y;
    int32_t     w;
    int32_t     h;
    int32_t     subpixel;
    const char  *maker;
    const char  *model;
    int32_t     transform;
};

struct pepper_output_mode
{
    uint32_t    flags;
    int32_t     width;
    int32_t     height;
    int32_t     refresh;
};

struct pepper_output_interface
{
    void *          (*create)(pepper_compositor_t *compositor, int32_t w, int32_t h, void *data);
    void            (*destroy)(void *output);

    int32_t         (*get_subpixel_order)(void *output);
    const char *    (*get_maker_name)(void *output);
    const char *    (*get_model_name)(void *output);
    int32_t         (*get_scale)(void *output);
    int             (*get_mode_count)(void *output);
    void            (*get_mode)(void *output, int index, pepper_output_mode_t *mode);
};

PEPPER_API pepper_output_t *
pepper_output_create(pepper_compositor_t *compositor,
                     int32_t x, int32_t y, int32_t w, int32_t h, int32_t transform, int32_t scale,
                     const pepper_output_interface_t *interface, void *data);

PEPPER_API pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output);

PEPPER_API void
pepper_output_destroy(pepper_output_t *output);

PEPPER_API void
pepper_output_move(pepper_output_t *output, int32_t x, int32_t y);

PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_output_t *output);

PEPPER_API unsigned int
pepper_output_get_scale(pepper_output_t *output);

PEPPER_API int
pepper_output_get_mode_count(pepper_output_t *output);

PEPPER_API const pepper_output_mode_t *
pepper_output_get_mode(pepper_output_t *output, int index);

PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_output_t *output, const pepper_output_mode_t *mode);

PEPPER_API void
pepper_output_update_mode(pepper_output_t *output);

/* Input. */

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_H */
