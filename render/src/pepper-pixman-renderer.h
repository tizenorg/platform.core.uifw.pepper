#ifndef PEPPER_PIXMAN_RENDERER_H
#define PEPPER_PIXMAN_RENDERER_H

#include <pepper-render.h>
#include <pixman.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_renderer_t *
pepper_pixman_renderer_create(pepper_object_t *compositor);

PEPPER_API pepper_render_target_t *
pepper_pixman_renderer_create_target(pepper_format_t format, void *pixels,
                                     int stride, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_PIXMAN_RENDERER_H */
