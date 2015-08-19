#ifndef PEPPER_INPUT_BACKEND_H
#define PEPPER_INPUT_BACKEND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_input_device_backend      pepper_input_device_backend_t;

struct pepper_input_device_backend
{
    const char *    (*get_property)(void *device, const char *key);
};

PEPPER_API pepper_input_device_t *
pepper_input_device_create(pepper_compositor_t *compositor, uint32_t caps,
                           const pepper_input_device_backend_t *backend, void *data);

PEPPER_API void
pepper_input_device_destroy(pepper_input_device_t *device);

PEPPER_API uint32_t
pepper_input_device_get_caps(pepper_input_device_t *device);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_INPUT_BACKEND_H */
