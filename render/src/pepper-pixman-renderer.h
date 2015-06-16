#ifndef PEPPER_PIXMAN_RENDERER_H
#define PEPPER_PIXMAN_RENDERER_H

#include <pepper-render.h>
#include <pixman.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_renderer_t *
pepper_pixman_renderer_create(pepper_object_t *compositor);

PEPPER_API void
pepper_pixman_renderer_set_target(pepper_renderer_t *r, pixman_image_t *image);

PEPPER_API pixman_image_t *
pepper_pixman_renderer_get_target(pepper_renderer_t *r);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_PIXMAN_RENDERER_H */
