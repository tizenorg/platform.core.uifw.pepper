/*
* Copyright © 2008-2012 Kristian Høgsberg
* Copyright © 2010-2012 Intel Corporation
* Copyright © 2011 Benjamin Franzke
* Copyright © 2012 Collabora, Ltd.
* Copyright © 2015 S-Core Corporation
* Copyright © 2015-2016 Samsung Electronics co., Ltd. All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

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
    pepper_list_init(&fbdev->output_list);

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
    if (fbdev->pixman_renderer)
        pepper_renderer_destroy(fbdev->pixman_renderer);

    if (!pepper_list_empty(&fbdev->output_list))
    {
        fbdev_output_t *output, *tmp;

        pepper_list_for_each_safe(output, tmp, &fbdev->output_list, link)
            pepper_fbdev_output_destroy(output);
    }

    free(fbdev);
}
