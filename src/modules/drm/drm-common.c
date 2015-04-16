#include "drm-internal.h"

PEPPER_API pepper_drm_t *
pepper_drm_create(pepper_compositor_t *compositor, const char *device)
{
    pepper_drm_t    *drm;

    drm = (pepper_drm_t *)pepper_calloc(1, sizeof(pepper_drm_t));
    if (!drm)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    drm->compositor = compositor;
    drm->input = pepper_libinput_create(compositor);

    if (!drm->input)
    {
        PEPPER_ERROR("Failed to create pepper_libinput in %s\n", __FUNCTION__);
        goto error;
    }

    /* TODO */

    return drm;

error:
    if (drm)
        pepper_free(drm);

    return NULL;
}

PEPPER_API void
pepper_drm_destroy(pepper_drm_t *drm)
{
    /* TODO */

    pepper_libinput_destroy(drm->input);
    pepper_free(drm);

    return;
}
