#include <libudev.h>
#include <unistd.h>
#include <stdlib.h>

#include "fbdev-internal.h"

PEPPER_API pepper_fbdev_t *
pepper_fbdev_create(pepper_object_t *compositor, const char *device, const char *renderer)
{
    pepper_fbdev_t *fbdev;

    fbdev = (pepper_fbdev_t *)calloc(1, sizeof(pepper_fbdev_t));
    if (!fbdev)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    fbdev->udev = udev_new();
    if (!fbdev->udev)
    {
        PEPPER_ERROR("Failed to create udev context in %s\n", __FUNCTION__);
        goto error;
    }

    fbdev->compositor = compositor;
    fbdev->input = pepper_libinput_create(compositor, fbdev->udev);

    if (!fbdev->input)
    {
        PEPPER_ERROR("Failed to create pepper_libinput in %s\n", __FUNCTION__);
        goto error;
    }

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

    if (!wl_list_empty(&fbdev->output_list))
    {
        wl_list_for_each_safe(output, next, &fbdev->output_list, link)
            pepper_fbdev_output_destroy(output);
    }

    if (fbdev->input)
        pepper_libinput_destroy(fbdev->input);

    if (fbdev->udev)
        udev_unref(fbdev->udev);

    free(fbdev);
}
