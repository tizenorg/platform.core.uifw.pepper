#ifndef PEPPER_RENDER_H
#define PEPPER_RENDER_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_renderer  pepper_renderer_t;

PEPPER_API void
pepper_renderer_destroy(pepper_renderer_t *renderer);

PEPPER_API void
pepper_renderer_repaint_output(pepper_renderer_t *renderer, pepper_output_t *output);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_RENDER_H */
