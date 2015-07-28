#include <libudev.h>
#include <unistd.h>
#include <stdlib.h>

#include "fbdev-internal.h"

PEPPER_API pepper_fbdev_t *
pepper_fbdev_create(pepper_compositor_t *compositor, struct udev *udev,
                    const char *device, const char *renderer)
{
    pepper_fbdev_t *fbdev;

    fbdev = (pepper_fbdev_t *)calloc(1, sizeof(pepper_fbdev_t));
    if (!fbdev)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    fbdev->pixman_renderer = pepper_pixman_renderer_create(compositor);
    if (!fbdev->pixman_renderer)
    {
        PEPPER_ERROR("Failed to create pixman renderer.\n");
        goto error;
    }

    fbdev->compositor = compositor;
    fbdev->udev = udev;
    wl_list_init(&fbdev->output_list);

    if (!pepper_fbdev_output_create(fbdev, renderer))
    {
        PEPPER_ERROR("Failed to connect fbdev output in %s\n", __FUNCTION__);
        goto error;
    }

    return fbdev;

error:
    if (fbdev)
        pepper_fbdev_destroy(fbdev);

    return NULL;
}

PEPPER_API void
pepper_fbdev_destroy(pepper_fbdev_t *fbdev)
{
    fbdev_output_t *output, *next;

    if (fbdev->pixman_renderer)
        pepper_renderer_destroy(fbdev->pixman_renderer);

    if (!wl_list_empty(&fbdev->output_list))
    {
        wl_list_for_each_safe(output, next, &fbdev->output_list, link)
            pepper_fbdev_output_destroy(output);
    }

    free(fbdev);
}
