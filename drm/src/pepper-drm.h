#ifndef PEPPER_DRM_H
#define PEPPER_DRM_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_drm   pepper_drm_t;

PEPPER_API pepper_drm_t *
pepper_drm_create(pepper_object_t *compositor, const char *device);

PEPPER_API void
pepper_drm_destroy(pepper_drm_t *drm);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_DRM_H */
