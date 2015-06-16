#ifndef PEPPER_WAYLAND_H
#define PEPPER_WAYLAND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_wayland   pepper_wayland_t;

PEPPER_API pepper_wayland_t *
pepper_wayland_connect(pepper_object_t *compositor, const char *socket_name);

PEPPER_API void
pepper_wayland_destroy(pepper_wayland_t *conn);

PEPPER_API pepper_object_t *
pepper_wayland_output_create(pepper_wayland_t *conn, int32_t w, int32_t h, const char *renderer);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_WAYLAND_H */
