#ifndef PEPPER_INPUT_BACKEND_H
#define PEPPER_INPUT_BACKEND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

PEPPER_API pepper_pointer_device_t *
pepper_pointer_device_create(pepper_compositor_t *compositor);

PEPPER_API void
pepper_pointer_device_destroy(pepper_pointer_device_t *device);

PEPPER_API pepper_keyboard_device_t *
pepper_keyboard_device_create(pepper_compositor_t *compositor);

PEPPER_API void
pepper_keyboard_device_destroy(pepper_keyboard_device_t *device);

PEPPER_API pepper_touch_device_t *
pepper_touch_device_create(pepper_compositor_t *compositor);

PEPPER_API void
pepper_touch_device_destroy(pepper_touch_device_t *device);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_INPUT_BACKEND_H */
