#ifndef PEPPER_GL_RENDERER_H
#define PEPPER_GL_RENDERER_H

#include "pepper.h"

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_renderer_t *
pepper_gl_renderer_create(pepper_compositor_t *compositor, void *display, void *window,
                          const char *platform, pepper_format_t format,
                          const uint32_t *native_visual_id);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_GL_RENDERER_H */
