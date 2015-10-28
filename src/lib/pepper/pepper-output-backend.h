#ifndef PEPPER_OUTPUT_BACKEND_H
#define PEPPER_OUTPUT_BACKEND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_plane             pepper_plane_t;
typedef struct pepper_output_backend    pepper_output_backend_t;
typedef struct pepper_render_item       pepper_render_item_t;

struct pepper_output_backend
{
    void            (*destroy)(void *output);

    int32_t         (*get_subpixel_order)(void *output);
    const char *    (*get_maker_name)(void *output);
    const char *    (*get_model_name)(void *output);

    int             (*get_mode_count)(void *output);
    void            (*get_mode)(void *output, int index, pepper_output_mode_t *mode);
    pepper_bool_t   (*set_mode)(void *output, const pepper_output_mode_t *mode);

    void            (*assign_planes)(void *output, const pepper_list_t *view_list);
    void            (*start_repaint_loop)(void *output);
    void            (*repaint)(void *output, const pepper_list_t *plane_list);
    void            (*attach_surface)(void *output, pepper_surface_t *surface, int *w, int *h);
    void            (*flush_surface_damage)(void *output, pepper_surface_t *surface);
};

PEPPER_API pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t *compositor,
                             const pepper_output_backend_t *backend, const char *name, void *data);

struct pepper_render_item
{
    pepper_view_t       *view;
    pepper_mat4_t       transform;
    pixman_region32_t   visible_region;
};

PEPPER_API pepper_plane_t *
pepper_output_add_plane(pepper_output_t *output, pepper_plane_t *above_plane);

PEPPER_API void
pepper_plane_destroy(pepper_plane_t *plane);

PEPPER_API pixman_region32_t *
pepper_plane_get_damage_region(pepper_plane_t *plane);

PEPPER_API pixman_region32_t *
pepper_plane_get_clip_region(pepper_plane_t *plane);

PEPPER_API const pepper_list_t *
pepper_plane_get_render_list(pepper_plane_t *plane);

PEPPER_API void
pepper_plane_subtract_damage_region(pepper_plane_t *plane, pixman_region32_t *damage);

PEPPER_API void
pepper_plane_clear_damage_region(pepper_plane_t *plane);

PEPPER_API void
pepper_view_assign_plane(pepper_view_t *view, pepper_output_t *output, pepper_plane_t *plane);

PEPPER_API void
pepper_output_add_damage_region(pepper_output_t *output, pixman_region32_t *region);

PEPPER_API void
pepper_output_finish_frame(pepper_output_t *output, struct timespec *ts);

PEPPER_API void
pepper_output_remove(pepper_output_t *output);

PEPPER_API void
pepper_output_update_mode(pepper_output_t *output);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_OUTPUT_BACKEND_H */
