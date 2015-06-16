#ifndef PEPPER_FBDEV_H
#define PEPPER_FBDEV_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pepper_fbdev   pepper_fbdev_t;

PEPPER_API pepper_fbdev_t *
pepper_fbdev_create(pepper_object_t *compositor, const char *device/*FIXME*/);

PEPPER_API void
pepper_fbdev_destroy(pepper_fbdev_t *fbdev);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_FBDEV_H */
