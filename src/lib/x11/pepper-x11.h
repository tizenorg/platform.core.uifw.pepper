#ifndef PEPPER_X11_H
#define PEPPER_X11_h

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_x11_connection pepper_x11_connection_t;

PEPPER_API pepper_x11_connection_t *
pepper_x11_connect(pepper_compositor_t *compositor, const char *display_name);

PEPPER_API void
pepper_x11_destroy(pepper_x11_connection_t *conn);

PEPPER_API pepper_output_t *
pepper_x11_output_create(pepper_x11_connection_t *connection, int32_t w, int32_t h,
                         const char *renderer);

PEPPER_API void
pepper_x11_seat_create(pepper_x11_connection_t* conn);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_X11_H */
