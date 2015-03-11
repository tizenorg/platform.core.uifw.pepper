#ifndef PEPPER_H
#define PEPPER_H

#include <stdint.h>

#define PEPPER_FALSE    0
#define PEPPER_TRUE     1

typedef uint32_t                    pepper_bool_t;
typedef struct pepper_compositor    pepper_compositor_t;

/* Compositor functions. */
pepper_compositor_t *
pepper_compositor_create(const char *socket_name,
                         const char *backend_name,
                         const char *input_name,
                         const char *shell_name,
                         const char *renderer_name);

void
pepper_compositor_destroy(pepper_compositor_t *compositor);

void
pepper_compositor_run(pepper_compositor_t *compositor);

#endif /* PEPPER_H */
