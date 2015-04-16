#ifndef DRM_INTERNAL_H
#define DRM_INTERNAL_H

#include <common.h>
#include <pepper-libinput.h>

#include "pepper-drm.h"

struct pepper_drm
{
    pepper_compositor_t        *compositor;
    pepper_libinput_t          *input;
};

#endif /* DRM_INTERNAL_H */
