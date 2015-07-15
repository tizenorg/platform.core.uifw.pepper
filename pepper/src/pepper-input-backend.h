#ifndef PEPPER_INPUT_BACKEND_H
#define PEPPER_INPUT_BACKEND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_seat_backend      pepper_seat_backend_t;

struct pepper_seat_backend
{
    void            (*destroy)(void *data);
    void            (*add_capabilities_listener)(void *data, struct wl_listener *listener);
    void            (*add_name_listener)(void *data, struct wl_listener *listener);

    uint32_t        (*get_capabilities)(void *data);
    const char *    (*get_name)(void *data);
};

PEPPER_API pepper_seat_t *
pepper_compositor_add_seat(pepper_compositor_t *compositor,
                           const pepper_seat_backend_t *backend,
                           void *data);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_INPUT_BACKEND_H */
