#include <libudev.h>
#include <unistd.h>
#include <stdlib.h>
#include <gbm.h>

#include "drm-internal.h"

PEPPER_API pepper_drm_t *
pepper_drm_create(pepper_compositor_t *compositor, struct udev *udev,
                  const char *device, const char *renderer)
{
    pepper_drm_t    *drm;

    drm = (pepper_drm_t *)calloc(1, sizeof(pepper_drm_t));
    if (!drm)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    drm->compositor = compositor;
    drm->udev = udev;
    wl_list_init(&drm->output_list);
    pepper_list_init(&drm->plane_list);

    if (!pepper_drm_output_create(drm, renderer))
    {
        PEPPER_ERROR("Failed to connect drm output in %s\n", __FUNCTION__);
        goto error;
    }

    return drm;

error:
    if (drm)
        pepper_drm_destroy(drm);

    return NULL;
}

PEPPER_API void
pepper_drm_destroy(pepper_drm_t *drm)
{
    if (!drm)
        return;

    pepper_drm_output_destroy(drm);
    free(drm);
}
