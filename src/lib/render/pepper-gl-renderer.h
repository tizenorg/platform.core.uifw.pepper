#ifndef PEPPER_GL_RENDERER_H
#define PEPPER_GL_RENDERER_H

#include <pepper-render.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_renderer_t *
pepper_gl_renderer_create(pepper_compositor_t *compositor, void *display, const char *platform);

PEPPER_API pepper_render_target_t *
pepper_gl_renderer_create_target(pepper_renderer_t *renderer, void *window, pepper_format_t format,
                                 const void *visual_id, int32_t width, int32_t height);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_GL_RENDERER_H */
