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

static void
fbdev_output_destroy(void *o)
{
    fbdev_output_t *output = (fbdev_output_t *)o;

    wl_signal_emit(&output->destroy_signal, NULL);
    wl_list_remove(&output->link);

    if (output->frame_done_timer)
        wl_event_source_remove(output->frame_done_timer);

    if (output->render_target)
        pepper_render_target_destroy(output->render_target);

    if (output->shadow_image)
        pixman_image_unref(output->shadow_image);

    if (output->frame_buffer_image)
        pixman_image_unref(output->frame_buffer_image);

    if (output->frame_buffer_pixels)
        munmap(output->frame_buffer_pixels, output->h * output->stride);

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
fbdev_output_assign_planes(void *o, const pepper_list_t *view_list)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    pepper_list_t  *l;

    PEPPER_LIST_FOR_EACH(view_list, l)
    {
        pepper_view_t *view = l->item;
        pepper_view_assign_plane(view, output->base, output->primary_plane);
    }
}

static void
fbdev_output_repaint(void *o, const pepper_list_t *plane_list)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    pepper_list_t  *l;

    PEPPER_LIST_FOR_EACH(plane_list, l)
    {
        pepper_plane_t *plane = l->item;

        if (plane == output->primary_plane)
        {
            const pepper_list_t *render_list = pepper_plane_get_render_list(plane);
            pixman_region32_t   *damage = pepper_plane_get_damage_region(plane);

            pepper_renderer_repaint_output(output->renderer, output->base, render_list, damage);
            pepper_plane_clear_damage_region(plane);

            /* FIXME: composite with damage? */
            if (output->use_shadow)
                pixman_image_composite32(PIXMAN_OP_SRC, output->shadow_image, NULL,
                                         output->frame_buffer_image, 0, 0, 0, 0, 0, 0,
                                         output->w, output->h);
        }
    }

    wl_event_source_timer_update(output->frame_done_timer, 16);
}

static void
fbdev_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
    pepper_renderer_attach_surface(((fbdev_output_t *)o)->renderer, surface, w, h);
}

static void
fbdev_output_add_frame_listener(void *o, struct wl_listener *listener)
{
    fbdev_output_t *output = (fbdev_output_t *)o;
    wl_signal_add(&output->frame_signal, listener);
}

struct pepper_output_backend fbdev_output_backend =
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

    fbdev_output_assign_planes,
    fbdev_output_repaint,
    fbdev_output_attach_surface,
    fbdev_output_add_frame_listener,
};

static pepper_bool_t
init_pixman_renderer(fbdev_output_t *output)
{
    pepper_render_target_t *target;

    if (!output->fbdev->pixman_renderer)
        return PEPPER_FALSE;

    if (output->use_shadow)
        target = pepper_pixman_renderer_create_target_for_image(output->shadow_image);
    else
        target = pepper_pixman_renderer_create_target(output->format, output->frame_buffer_pixels,
                                                      output->stride, output->w, output->h);

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

static int
frame_done_handler(void* data)
{
    fbdev_output_t *output = (fbdev_output_t *)data;
    wl_signal_emit(&output->frame_signal, NULL);

    return 1;
}

pepper_bool_t
pepper_fbdev_output_create(pepper_fbdev_t *fbdev, const char *renderer)
{
    fbdev_output_t             *output = NULL;
    int                         fd;
    struct fb_fix_screeninfo    fixed_info;
    struct fb_var_screeninfo    var_info;

    struct wl_display          *display;
    struct wl_event_loop       *loop;

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

    output->frame_buffer_pixels = mmap(NULL, output->h * output->stride,
                                       PROT_WRITE, MAP_SHARED, fd, 0);
    if (!output->frame_buffer_pixels)
    {
        PEPPER_ERROR("mmap failed.\n");
        goto error;
    }

    close(fd);

    /* TODO: read & set output->use_shadow value from somewhere */
    output->use_shadow = PEPPER_TRUE;
    if (output->use_shadow)
    {
        pixman_format_code_t pixman_format = get_pixman_format(output->format);

        output->frame_buffer_image = pixman_image_create_bits(pixman_format,
                                                              output->w, output->h,
                                                              output->frame_buffer_pixels,
                                                              output->stride);
        if (!output->frame_buffer_image)
        {
            PEPPER_ERROR("Failed to create pixman image in %s: pixels_image\n", __FUNCTION__);
            goto error;
        }

        output->shadow_image = pixman_image_create_bits(pixman_format, output->w, output->h,
                                                        NULL, output->stride);
        if (!output->shadow_image)
        {
            PEPPER_ERROR("Failed to create pixman image in %s: shadow_image\n", __FUNCTION__);
            goto error;
        }
    }

     if (!init_renderer(output, renderer))
    {
        PEPPER_ERROR("Failed to initialize renderer in %s\n", __FUNCTION__);
        goto error;
    }

    output->base = pepper_compositor_add_output(output->fbdev->compositor,
                                                &fbdev_output_backend, "fbdev", output);
    if (!output->base)
    {
        PEPPER_ERROR("Failed to add output to compositor in %s\n", __FUNCTION__);
        goto error;
    }

    output->primary_plane = pepper_output_add_plane(output->base, NULL);
    wl_list_insert(&fbdev->output_list, &output->link);

    display = pepper_compositor_get_display(fbdev->compositor);
    loop = wl_display_get_event_loop(display);
    output->frame_done_timer = wl_event_loop_add_timer(loop, frame_done_handler, output);

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
