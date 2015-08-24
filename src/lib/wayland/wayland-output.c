#include "wayland-internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pepper-pixman-renderer.h>

#if ENABLE_WAYLAND_BACKEND_EGL && ENABLE_GL_RENDERER
#include <pepper-gl-renderer.h>
#endif

static const char *maker_name = "wayland";
static const char *model_name = "wayland";

static void
shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void
shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges,
                        int32_t w, int32_t h)
{
    wayland_output_t       *output = data;
    wayland_shm_buffer_t   *buffer, *next;

    output->w = w;
    output->h = h;

    /* Destroy free buffers immediately. */
    wl_list_for_each_safe(buffer, next, &output->shm.free_buffers, link)
        wayland_shm_buffer_destroy(buffer);

    /* Orphan attached buffers. They will be destroyed when the compositor releases them. */
    wl_list_for_each_safe(buffer, next, &output->shm.attached_buffers, link)
    {
        buffer->output = NULL;
        wl_list_remove(&buffer->link);
    }

    PEPPER_ASSERT(wl_list_empty(&output->shm.free_buffers));
    PEPPER_ASSERT(wl_list_empty(&output->shm.attached_buffers));

    pepper_output_update_mode(output->base);
}

static void
shell_surface_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener =
{
    shell_surface_ping,
    shell_surface_configure,
    shell_surface_popup_done,
};

static void
wayland_output_destroy(void *o)
{
    wayland_output_t *output = o;

    wl_list_remove(&output->conn_destroy_listener.link);

    wl_surface_destroy(output->surface);
    wl_shell_surface_destroy(output->shell_surface);

    free(output);
}

static int32_t
wayland_output_get_subpixel_order(void *o)
{
    wayland_output_t *output = o;
    return output->subpixel;
}

static const char *
wayland_output_get_maker_name(void *o)
{
    return maker_name;
}

static const char *
wayland_output_get_model_name(void *o)
{
    return model_name;
}

static int
wayland_output_get_mode_count(void *o)
{
    /* There's only one available mode in wayland backend which is also the current mode. */
    return 1;
}

static void
wayland_output_get_mode(void *o, int index, pepper_output_mode_t *mode)
{
    wayland_output_t *output = o;

    if (index != 0)
        return;

    mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
    mode->w = output->w;
    mode->h = output->h;
    mode->refresh = 60000;
}

static pepper_bool_t
wayland_output_set_mode(void *o, const pepper_output_mode_t *mode)
{
    wayland_output_t *output = o;

    if (mode->w <= 0 || mode->h <= 0)
        return PEPPER_FALSE;

    if (mode->refresh != 60000)
        return PEPPER_FALSE;

    if (output->w != mode->w || output->h != mode->h)
    {
        output->w = mode->w;
        output->h = mode->h;

        /* TODO: Handle resize here. */

        pepper_output_update_mode(output->base);
    }

    return PEPPER_TRUE;
}

static void
frame_done(void *data, struct wl_callback *callback, uint32_t time)
{
    wayland_output_t *output = data;

    wl_callback_destroy(callback);
    pepper_output_finish_frame(output->base, NULL);
}

static const struct wl_callback_listener frame_listener =
{
    frame_done,
};

static void
wayland_output_assign_planes(void *o, const pepper_list_t *view_list)
{
    wayland_output_t   *output = (wayland_output_t *)o;
    pepper_list_t      *l;

    PEPPER_LIST_FOR_EACH(view_list, l)
    {
        pepper_view_t *view = l->item;
        pepper_view_assign_plane(view, output->base, output->primary_plane);
    }
}

static void
wayland_output_repaint(void *o, const pepper_list_t *plane_list)
{
    wayland_output_t   *output = o;
    struct wl_callback *callback;

    pepper_list_t  *l;

    PEPPER_LIST_FOR_EACH(plane_list, l)
    {
        pepper_plane_t *plane = l->item;

        if (plane == output->primary_plane)
        {
            const pepper_list_t *render_list = pepper_plane_get_render_list(plane);
            pixman_region32_t   *damage = pepper_plane_get_damage_region(plane);

            if (output->render_pre)
                output->render_pre(output);

            pepper_renderer_repaint_output(output->renderer, output->base, render_list, damage);
            pepper_plane_clear_damage_region(plane);

            if (output->render_post)
                output->render_post(output);

            callback = wl_surface_frame(output->surface);
            wl_callback_add_listener(callback, &frame_listener, output);
            wl_surface_commit(output->surface);
            wl_display_flush(output->conn->display);
        }
    }
}

static void
wayland_output_attach_surface(void *o, pepper_surface_t *surface, int *w, int *h)
{
    pepper_renderer_attach_surface(((wayland_output_t *)o)->renderer, surface, w, h);
}

static const pepper_output_backend_t wayland_output_backend =
{
    wayland_output_destroy,

    wayland_output_get_subpixel_order,
    wayland_output_get_maker_name,
    wayland_output_get_model_name,

    wayland_output_get_mode_count,
    wayland_output_get_mode,
    wayland_output_set_mode,

    wayland_output_assign_planes,
    wayland_output_repaint,
    wayland_output_attach_surface,
};

static void
pixman_render_pre(wayland_output_t *output)
{
    wayland_shm_buffer_t *buffer = NULL;

    if (wl_list_empty(&output->shm.free_buffers))
    {
        buffer = wayland_shm_buffer_create(output);
        if (!buffer)
        {
            PEPPER_ERROR("Failed to create a shm buffer.\n");
            return;
        }
    }
    else
    {
        buffer = pepper_container_of(output->shm.free_buffers.next, wayland_shm_buffer_t, link);
        wl_list_remove(&buffer->link);
    }

    wl_list_insert(output->shm.attached_buffers.prev, &buffer->link);
    output->shm.current_buffer = buffer;

    pepper_renderer_set_target(output->renderer, buffer->target);
}

static void
pixman_render_post(wayland_output_t *output)
{
    wl_surface_attach(output->surface, output->shm.current_buffer->buffer, 0, 0);
    wl_surface_damage(output->surface, 0, 0, output->w, output->h);
}

static pepper_bool_t
init_gl_renderer(wayland_output_t *output)
{
#if ENABLE_WAYLAND_BACKEND_EGL
    if (!output->connection->gl_renderer)
        return PEPPER_FALSE;

    output->egl.window = wl_egl_window_create(output->surface, output->w, output->h);
    if (!output->egl.window)
        return PEPPER_FALSE;

    output->gl_render_target = pepper_gl_renderer_create_target(output->conn->renderer,
                                                                output->egl.window,
                                                                PEPPER_FORMAT_ARGB8888, NULL);

    if (!output->gl_render_target)
    {
        wl_egl_window_destroy(output->egl.window);
        return PEPPER_FALSE;
    }

    output->render_target = output->gl_render_target;
    output->renderer = output->conn->renderer;

    return PEPPER_TRUE;
#else
    return PEPPER_FALSE;
#endif
}

static pepper_bool_t
init_pixman_renderer(wayland_output_t *output)
{
    if (!output->conn->pixman_renderer)
        return PEPPER_FALSE;

    wl_list_init(&output->shm.free_buffers);
    wl_list_init(&output->shm.attached_buffers);

    output->renderer    = output->conn->pixman_renderer;
    output->render_pre  = pixman_render_pre;
    output->render_post = pixman_render_post;

    return PEPPER_TRUE;
}

static pepper_bool_t
init_renderer(wayland_output_t *output, const char *name)
{
    if (strcmp(name, "gl") == 0)
    {
        return init_gl_renderer(output);
    }

    return init_pixman_renderer(output);
}

PEPPER_API pepper_output_t *
pepper_wayland_output_create(pepper_wayland_t *conn, int32_t w, int32_t h, const char *renderer)
{
    wayland_output_t   *output;

    output = calloc(1, sizeof(wayland_output_t));
    if (!output)
        return NULL;

    output->conn = conn;
    output->w = w;
    output->h = h;

    /* Hard-Coded: subpixel order to horizontal RGB. */
    output->subpixel = WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;

    /* Create wayland resources. */
    output->surface = wl_compositor_create_surface(conn->compositor);
    output->shell_surface = wl_shell_get_shell_surface(conn->shell, output->surface);
    wl_shell_surface_add_listener(output->shell_surface, &shell_surface_listener, output);
    wl_shell_surface_set_toplevel(output->shell_surface);
    snprintf(&output->name[0], 32, "wayland-%p", output);

    /* Add compositor base class output object for this output. */
    output->base = pepper_compositor_add_output(conn->pepper, &wayland_output_backend,
                                                output->name, output);
    if (!output->base)
    {
        wayland_output_destroy(output);
        return NULL;
    }

    /* Create renderer. */
    if (!init_renderer(output, renderer))
    {
        wayland_output_destroy(output);
        return NULL;
    }

    output->primary_plane = pepper_output_add_plane(output->base, NULL);
    wl_list_insert(&conn->output_list, &output->link);

    return output->base;
}
