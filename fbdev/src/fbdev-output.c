#include <fcntl.h>
#include <libudev.h>
#include <linux/fb.h>
#include <pixman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pepper-pixman-renderer.h>
#include <pepper-gl-renderer.h>

#include "fbdev-internal.h"

static void
fbdev_debug_print_fixed_screeninfo(const struct fb_fix_screeninfo *info)
{
    printf("Framebuffer ID: %s\n", info->id);
    printf("Framebuffer type: ");

    switch (info->type)
    {
    case FB_TYPE_PACKED_PIXELS:
        printf("packed pixels\n");
        break;
    case FB_TYPE_PLANES:
        printf("planar (non-interleaved)\n");
        break;
    case FB_TYPE_INTERLEAVED_PLANES:
        printf("planar (interleaved)\n");
        break;
    case FB_TYPE_TEXT:
        printf("text (not a framebuffer)\n");
        break;
    case FB_TYPE_VGA_PLANES:
        printf("planar (EGA/VGA)\n");
        break;
    default:
        printf("?????\n");
    }

    printf("Bytes per scanline: %i\n", info->line_length);
    printf("Visual type: ");

    switch (info->visual)
    {
    case FB_VISUAL_TRUECOLOR:
        printf("truecolor\n");
        break;
    case FB_VISUAL_PSEUDOCOLOR:
        printf("pseudocolor\n");
        break;
    case FB_VISUAL_DIRECTCOLOR:
        printf("directcolor\n");
        break;
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
        printf("fixed pseudocolor\n");
        break;
    default:
        printf("?????\n");
    }
}

static void
fbdev_debug_print_var_screeninfo(const struct fb_var_screeninfo *info)
{
    printf("Bits per pixel:     %i\n", info->bits_per_pixel);
    printf("Resolution:         %ix%i (virtual %ix%i)\n",
           info->xres, info->yres,
           info->xres_virtual, info->yres_virtual);
    printf("Scrolling offset:   (%i,%i)\n",
           info->xoffset, info->yoffset);
    printf("Trans channel:      %i bits at offset %i\n",
           info->transp.length, info->transp.offset);
    printf("Red channel:        %i bits at offset %i\n",
           info->red.length, info->red.offset);
    printf("Green channel:      %i bits at offset %i\n",
           info->red.length, info->green.offset);
    printf("Blue channel:       %i bits at offset %i\n",
           info->red.length, info->blue.offset);
}
void
fbdev_output_destroy(void *o)
{
    fbdev_output_t *output = (fbdev_output_t *)o;

    wl_signal_emit(&output->destroy_signal, NULL);
    wl_list_remove(&output->link);

    if (output->render_target)
        pepper_render_target_destroy(output->render_target);

    if (output->pixels)
        munmap(output->pixels, output->w * output->stride);

    free(output);
}

static void
fbdev_output_add_destroy_listener(void *o, struct wl_listener *listener)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    wl_signal_add(&output->destroy_signal, listener);
}

static void
fbdev_output_add_mode_change_listener(void *o, struct wl_listener *listener)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    wl_signal_add(&output->mode_change_signal, listener);
}

static int32_t
fbdev_output_get_subpixel_order(void *o)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    return output->subpixel;
}

static const char *
fbdev_output_get_maker_name(void *o)
{
    return "PePPer FBDEV";
}

static const char *
fbdev_output_get_model_name(void *o)
{
    return "PePPer FBDEV";
}

static int
fbdev_output_get_mode_count(void *o)
{
    return 1;
}

static void
fbdev_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    fbdev_output_t *output = (fbdev_output_t *)o;

     if (index != 0)
        return;

    mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    mode->w = output->w;
    mode->h = output->h;
    mode->refresh = 60000;
}

static pepper_bool_t
fbdev_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    return PEPPER_FALSE;
}

static void
fbdev_output_repaint(void *o, const pepper_list_t *view_list, const pixman_region32_t *damage)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    pepper_renderer_repaint_output(output->renderer, output->base, view_list, damage);
}

static void
fbdev_output_add_frame_listener(void *o, struct wl_listener *listener)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    wl_signal_add(&output->frame_signal, listener);
}

struct pepper_output_interface fbdev_output_interface =
{
    fbdev_output_destroy,
    fbdev_output_add_destroy_listener,
    fbdev_output_add_mode_change_listener,

    fbdev_output_get_subpixel_order,
    fbdev_output_get_maker_name,
    fbdev_output_get_model_name,

    fbdev_output_get_mode_count,
    fbdev_output_get_mode,
    fbdev_output_set_mode,

    fbdev_output_repaint,
    fbdev_output_add_frame_listener,
};

static pepper_bool_t
init_pixman_renderer(fbdev_output_t *output)
{
    pepper_render_target_t *target;

    if (!output->fbdev->pixman_renderer)
        return PEPPER_FALSE;

    target = pepper_pixman_renderer_create_target(output->format, output->pixels, output->stride,
                                                  output->w, output->h);
    if (!target)
        return PEPPER_FALSE;

    output->renderer = output->fbdev->pixman_renderer;
    output->render_target = target;
    pepper_renderer_set_target(output->renderer, output->render_target);

    return PEPPER_TRUE;
}

static pepper_bool_t
init_renderer(fbdev_output_t *output, const char *renderer)
{
    /* Only support pixman renderer currently. */
    if (strcmp("pixman", renderer) != 0)
        return PEPPER_FALSE;

    return init_pixman_renderer(output);
}

pepper_bool_t
pepper_fbdev_output_create(pepper_fbdev_t *fbdev, const char *renderer)
{
    fbdev_output_t             *output = NULL;
    int                         fd;
    struct fb_fix_screeninfo    fixed_info;
    struct fb_var_screeninfo    var_info;

    /* fbdev open */
    fd = open("/dev/fb0"/*FIXME*/, O_RDWR | O_CLOEXEC);
    if (fd < 0)
    {
        PEPPER_ERROR("Failed to open fbdev in %s\n", __FUNCTION__);
        goto error;
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info) < 0)
    {
        PEPPER_ERROR("Failed to get fixed screen info in %s\n", __FUNCTION__);
        goto error;
    }

    fbdev_debug_print_fixed_screeninfo(&fixed_info);

    if (ioctl(fd, FBIOGET_VSCREENINFO, &var_info) < 0)
    {
        PEPPER_ERROR("Failed to get variable screen info in %s\n", __FUNCTION__);
        goto error;
    }

    fbdev_debug_print_var_screeninfo(&var_info);

    output = (fbdev_output_t *)calloc(1, sizeof(fbdev_output_t));
    if (!output)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    output->fbdev = fbdev;
    wl_list_init(&output->link);

    output->format = PEPPER_FORMAT_XRGB8888;
    output->subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
    output->w = var_info.xres;
    output->h = var_info.yres;
    output->bpp = var_info.bits_per_pixel;
    output->stride = output->w * (output->bpp / 8);

    wl_signal_init(&output->destroy_signal);
    wl_signal_init(&output->mode_change_signal);
    wl_signal_init(&output->frame_signal);

    output->pixels = mmap(NULL, output->h * output->stride, PROT_WRITE, MAP_SHARED, fd, 0);
    if (!output->pixels)
    {
        PEPPER_ERROR("mmap failed.\n");
        goto error;
    }

    close(fd);

    if (!init_renderer(output, renderer))
    {
        PEPPER_ERROR("Failed to initialize renderer in %s\n", __FUNCTION__);
        goto error;
    }

    output->base = pepper_compositor_add_output(output->fbdev->compositor,
                                                &fbdev_output_interface, output);
    if (!output->base)
    {
        PEPPER_ERROR("Failed to add output to compositor in %s\n", __FUNCTION__);
        goto error;
    }

    wl_list_insert(&fbdev->output_list, &output->link);
    return PEPPER_TRUE;

error:
    if (output)
        pepper_fbdev_output_destroy(output);

    if (fd >= 0)
        close(fd);

    return PEPPER_FALSE;
}

void
pepper_fbdev_output_destroy(fbdev_output_t *output)
{
    fbdev_output_destroy(output);
}
