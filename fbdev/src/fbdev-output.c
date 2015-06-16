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

#define USE_PIXMAN  1   /* FIXME */

static pepper_bool_t
init_renderer(fbdev_output_t *output);

static void
fini_renderer(fbdev_output_t *output);

void
fbdev_output_destroy(void *o)
{
    fbdev_output_t *output = (fbdev_output_t *)o;

    wl_list_remove(&output->link);
    fini_renderer(output);

    if (output->fb_image)
        pixman_image_unref(output->fb_image);

    if (output->fb)
        munmap(output->fb, output->w * output->h * (output->bits_per_pixel / 8));

    wl_signal_emit(&output->destroy_signal, NULL);
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
draw_pixman(fbdev_output_t *output)
{
    /* FIXME: copy shadow image to fb? */
    pepper_renderer_repaint_output(output->renderer, output->base);
}

static void
draw(fbdev_output_t *output)
{
    if (USE_PIXMAN)
        draw_pixman(output);
}

static void
fbdev_output_repaint(void *o)
{
    fbdev_output_t *output = (fbdev_output_t *)o;

    draw(output);

    /* TODO */
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

static int
fbdev_open(const char *path, int flags)
{
    int fd;

    fd = open(path, flags | O_CLOEXEC);
    if (fd == -1)
        PEPPER_ERROR("Failed to open file[%s] in %s\n", path, __FUNCTION__);

    return fd;
}

static void
fini_pixman_renderer(fbdev_output_t *output)
{
    if (output->renderer)
        pepper_renderer_destroy(output->renderer);
}

static pepper_bool_t
init_pixman_renderer(fbdev_output_t *output)
{
    output->renderer = pepper_pixman_renderer_create(output->fbdev->compositor);
    if (!output->renderer)
    {
        PEPPER_ERROR("Failed to create pixman renderer in %s\n", __FUNCTION__);
        goto error;
    }

    /* FIXME: use shadow image? */
    pepper_pixman_renderer_set_target(output->renderer, output->fb_image);

    return PEPPER_TRUE;

error:

    fini_pixman_renderer(output);

    return PEPPER_FALSE;
}

static pepper_bool_t
init_renderer(fbdev_output_t *output)
{
    if (USE_PIXMAN)
        return init_pixman_renderer(output);
    else
        return PEPPER_FALSE;
}

static void
fini_renderer(fbdev_output_t *output)
{
    if (USE_PIXMAN)
        fini_pixman_renderer(output);
}

pepper_bool_t
pepper_fbdev_output_create(pepper_fbdev_t *fbdev)
{
    fbdev_output_t             *output = NULL;

    int                         fd;
    struct fb_fix_screeninfo    fixed_info;
    struct fb_var_screeninfo    var_info;

    /* fbdev open */
    fd = fbdev_open("/dev/fb0"/*FIXME*/, O_RDWR);
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

#if 1   /* print out some of the fixed info */
    {
        printf("Framebuffer ID: %s\n", fixed_info.id);
        printf("Framebuffer type: ");
        switch (fixed_info.type)
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
        printf("Bytes per scanline: %i\n", fixed_info.line_length);
        printf("Visual type: ");
        switch (fixed_info.visual)
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
#endif

    if (ioctl(fd, FBIOGET_VSCREENINFO, &var_info) < 0)
    {
        PEPPER_ERROR("Failed to get variable screen info in %s\n", __FUNCTION__);
        goto error;
    }

#if 1   /* print out some info */
    {
        printf("Bits per pixel:     %i\n", var_info.bits_per_pixel);
        printf("Resolution:         %ix%i (virtual %ix%i)\n",
               var_info.xres, var_info.yres,
               var_info.xres_virtual, var_info.yres_virtual);
        printf("Scrolling offset:   (%i,%i)\n",
               var_info.xoffset, var_info.yoffset);
        printf("Red channel:        %i bits at offset %i\n",
               var_info.red.length, var_info.red.offset);
        printf("Green channel:      %i bits at offset %i\n",
               var_info.red.length, var_info.green.offset);
        printf("Blue channel:       %i bits at offset %i\n",
               var_info.red.length, var_info.blue.offset);
    }
#endif

    output = (fbdev_output_t *)calloc(1, sizeof(fbdev_output_t));
    if (!output)
    {
        PEPPER_ERROR("Failed to allocate memory in %s\n", __FUNCTION__);
        goto error;
    }

    output->fbdev = fbdev;

    output->subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
    output->w = var_info.xres;
    output->h = var_info.yres;
    output->pixel_format = PIXMAN_x8r8g8b8; /*FIXME*/
    output->bits_per_pixel = var_info.bits_per_pixel;

    wl_signal_init(&output->destroy_signal);
    wl_signal_init(&output->mode_change_signal);
    wl_signal_init(&output->frame_signal);

    wl_list_insert(&fbdev->output_list, &output->link);

    output->fb = mmap(NULL, output->w * output->h * (output->bits_per_pixel / 8),
                      PROT_WRITE, MAP_SHARED, fd, 0);
    if (!output->fb)
    {
        PEPPER_ERROR("Failed to mmap in %s\n", __FUNCTION__);
        goto error;
    }

    output->fb_image = pixman_image_create_bits(output->pixel_format,
                                                output->w, output->h, output->fb,
                                                output->w * (output->bits_per_pixel / 8));
    if (!output->fb_image)
    {
        PEPPER_ERROR("Failed to create pixman image in %s\n", __FUNCTION__);
        goto error;
    }

    /* TODO : make shadow image? */

    output->base = pepper_compositor_add_output(output->fbdev->compositor,
                                                &fbdev_output_interface, output);
    if (!output->base)
    {
        PEPPER_ERROR("Failed to add output to compositor in %s\n", __FUNCTION__);
        goto error;
    }

    if (!init_renderer(output))
    {
        PEPPER_ERROR("Failed to initialize renderer in %s\n", __FUNCTION__);
        goto error;
    }

    if (fd >=0)
        close(fd);

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
