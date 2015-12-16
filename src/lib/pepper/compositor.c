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

#include "pepper-internal.h"

static void
compositor_create_surface(struct wl_client   *client,
                          struct wl_resource *resource,
                          uint32_t            id)
{
    pepper_compositor_t *compositor = wl_resource_get_user_data(resource);

    if (!pepper_surface_create(compositor, client, resource, id))
        wl_resource_post_no_memory(resource);
}

static void
compositor_create_region(struct wl_client   *client,
                         struct wl_resource *resource,
                         uint32_t            id)
{
    pepper_compositor_t *compositor = wl_resource_get_user_data(resource);

    if (!pepper_region_create(compositor, client, resource, id))
        wl_resource_post_no_memory(resource);
}

static const struct wl_compositor_interface compositor_interface =
{
    compositor_create_surface,
    compositor_create_region
};

static void
unbind_resource(struct wl_resource *resource)
{
    wl_list_remove(wl_resource_get_link(resource));
}

static void
compositor_bind(struct wl_client *client,
                void             *data,
                uint32_t          version,
                uint32_t          id)
{
    pepper_compositor_t *compositor = (pepper_compositor_t *)data;
    struct wl_resource  *resource;

    resource = wl_resource_create(client, &wl_compositor_interface, version, id);

    if (!resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        wl_client_post_no_memory(client);
        return;
    }

    wl_list_insert(&compositor->resource_list, wl_resource_get_link(resource));
    wl_resource_set_implementation(resource, &compositor_interface, compositor, unbind_resource);
}

void
pepper_compositor_schedule_repaint(pepper_compositor_t *compositor)
{
    pepper_output_t *output;

    pepper_list_for_each(output, &compositor->output_list, link)
        pepper_output_schedule_repaint(output);
}

/**
 * Create a compositor with the given fd and socket name
 *
 * @param socket_name   socket name, if NULL, automatically determined internally
 * @param fd            file descriptor to the socket, if -1, socket is created internally
 *
 * @return created compositor object
 *
 * On tizen, there're some security issues so that creating a socket is not allowed to applications.
 * In that situation, creating a compositor from already existing socket fd is required. The fd is
 * acquired by requesting to some kind of system service rather than creating directly by the
 * application.
 *
 * @see pepper_compositor_create()
 */
PEPPER_API pepper_compositor_t *
pepper_compositor_create_fd(const char *socket_name, int fd)
{
    int                  ret;
    pepper_compositor_t *compositor;

    compositor = (pepper_compositor_t *)pepper_object_alloc(PEPPER_OBJECT_COMPOSITOR,
                                                            sizeof(pepper_compositor_t));
    PEPPER_CHECK(compositor, goto error, "pepper_object_alloc() failed.\n");

    wl_list_init(&compositor->resource_list);

    pepper_list_init(&compositor->surface_list);
    pepper_list_init(&compositor->view_list);
    pepper_list_init(&compositor->region_list);
    pepper_list_init(&compositor->seat_list);
    pepper_list_init(&compositor->output_list);
    pepper_list_init(&compositor->input_device_list);

    compositor->display = wl_display_create();
    PEPPER_CHECK(compositor->display, goto error, "wl_display_create() failed.\n");

    if (socket_name)
    {
#if ENABLE_SOCKET_FD
        ret = wl_display_add_socket_fd(compositor->display, socket_name, fd);
#else
        ret = wl_display_add_socket(compositor->display, socket_name);
#endif
        PEPPER_CHECK(ret == 0, goto error, "wl_display_add_socket(name = %s) failed.\n", socket_name);
    }
    else
    {
        if (fd != -1 && socket_name == NULL)
            PEPPER_CHECK(socket_name, goto error, "pepper_compositor_create_fd()cannot support NULL socket name.\n");

        socket_name = wl_display_add_socket_auto(compositor->display);
        PEPPER_CHECK(socket_name, goto error, "wl_display_add_socket_auto() failed.\n");
    }

    compositor->socket_name = strdup(socket_name);
    PEPPER_CHECK(compositor->socket_name, goto error, "strdup() failed.\n");

    ret = wl_display_init_shm(compositor->display);
    PEPPER_CHECK(ret == 0, goto error, "wl_display_init_shm() failed.\n");

    compositor->global = wl_global_create(compositor->display, &wl_compositor_interface,
                                          3, compositor, compositor_bind);
    PEPPER_CHECK(compositor->global, goto error, "wl_global_create() failed.\n");

    ret = pepper_data_device_manager_init(compositor->display);
    PEPPER_CHECK(ret == PEPPER_TRUE, goto error, "pepper_data_device_manager_init() failed.\n");

    compositor->subcomp = pepper_subcompositor_create(compositor);
    PEPPER_CHECK(compositor->subcomp, goto error, "pepper_subcompositor_create() failed.\n");

    compositor->clock_id = CLOCK_MONOTONIC;
    return compositor;

error:
    if (compositor)
        pepper_compositor_destroy(compositor);

    return NULL;
}


/**
 * Create a compositor with the given socket name
 *
 * @param socket_name   socket name, if NULL, automatically determined internally
 *
 * @return created compositor object
 *
 * After creating a compositor, clients can connect to the socket.
 * wl_compositor global is internally created.
 *
 * @see pepper_compositor_destroy()
 */
PEPPER_API pepper_compositor_t *
pepper_compositor_create(const char *socket_name)
{
    return pepper_compositor_create_fd(socket_name, -1);
}

/**
 * Destroy the given compositor
 *
 * @param compositor    compositor object
 *
 * @see pepper_compositor_create()
 * @see pepper_compositor_create_fd()
 */
PEPPER_API void
pepper_compositor_destroy(pepper_compositor_t *compositor)
{
    pepper_surface_t        *surface, *next_surface;
    pepper_region_t         *region, *next_region;

    pepper_list_for_each_safe(surface, next_surface, &compositor->surface_list, link)
        pepper_surface_destroy(surface);

    pepper_list_for_each_safe(region, next_region, &compositor->region_list, link)
        pepper_region_destroy(region);

    if (compositor->subcomp)
        pepper_subcompositor_destroy(compositor->subcomp);

    if (compositor->socket_name)
        free(compositor->socket_name);

    if (compositor->global)
        wl_global_destroy(compositor->global);

    if (compositor->display)
        wl_display_destroy(compositor->display);

    pepper_object_fini(&compositor->base);
    free(compositor);
}

/**
 * Get the wl_display of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return wl_display of the compositor
 *
 * Exposing wl_display of a compositor is useful, as user can construct their own main loop and
 * install desired globals to provide extension protocols.
 */
PEPPER_API struct wl_display *
pepper_compositor_get_display(pepper_compositor_t *compositor)
{
    return compositor->display;
}

/**
 * Get the list of bound wl_resource of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return list of the bound wl_resource
 */
PEPPER_API struct wl_list *
pepper_compositor_get_resource_list(pepper_compositor_t *compositor)
{
    return &compositor->resource_list;
}

/**
 * Get the socket name of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return null terminating string of the socket name
 */
PEPPER_API const char *
pepper_compositor_get_socket_name(pepper_compositor_t *compositor)
{
    return compositor->socket_name;
}

/**
 * Get the list of outputs of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return list of outputs
 */
PEPPER_API const pepper_list_t *
pepper_compositor_get_output_list(pepper_compositor_t *compositor)
{
    return &compositor->output_list;
}

/**
 * Get the list of surfaces of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return list of surfaces
 */
PEPPER_API const pepper_list_t *
pepper_compositor_get_surface_list(pepper_compositor_t *compositor)
{
    return &compositor->surface_list;
}

/**
 * Get the list of views of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return list of views sorted in z-order (front to back)
 */
PEPPER_API const pepper_list_t *
pepper_compositor_get_view_list(pepper_compositor_t *compositor)
{
    return &compositor->view_list;
}

/**
 * Get the list of seats of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return list of seats
 */
PEPPER_API const pepper_list_t *
pepper_compositor_get_seat_list(pepper_compositor_t *compositor)
{
    return &compositor->seat_list;
}

/**
 * Get the list of input devices of the given compositor
 *
 * @param compositor    compositor object
 *
 * @return list of input devices
 */
PEPPER_API const pepper_list_t *
pepper_compositor_get_input_device_list(pepper_compositor_t *compositor)
{
    return &compositor->input_device_list;
}

/**
 * Pick a view for given position in the compositor
 *
 * @param compositor    compositor object
 * @param x             x coordinate in global space
 * @param y             y coordinate in global space
 * @param vx            pointer to receive x coordinate in view space
 * @param vx            pointer to receive y coordinate in view space
 *
 * @return picked view if there's any, NULL otherwise
 *
 * Pepper picks a view which is located topmost for the given position. If input region is set for
 * the surface of the view, the region should contain the position.
 */
PEPPER_API pepper_view_t *
pepper_compositor_pick_view(pepper_compositor_t *compositor,
                            double x, double y, double *vx, double *vy)
{
    pepper_view_t  *view;
    int             ix = (int)x;
    int             iy = (int)y;

    pepper_list_for_each(view, &compositor->view_list, compositor_link)
    {
        double  lx, ly;
        int     ilx, ily;

        if (!view->surface)
            continue;

        if (!pixman_region32_contains_point(&view->bounding_region, ix, iy, NULL))
            continue;

        pepper_view_get_local_coordinate(view, x, y, &lx, &ly);

        ilx = (int)lx;
        ily = (int)ly;

        if (ilx < 0 || ily < 0 || ilx >= view->w || ily >= view->h)
            continue;

        if (!pixman_region32_contains_point(&view->surface->input_region, ilx, ily, NULL))
            continue;

        if (vx)
            *vx = lx;

        if (vy)
            *vy = ly;

        return view;
    }

    return NULL;
}

/**
 * Set clock id for clock_gettime() function of the given pepper_compositor_t
 *
 * @param compositor    compositor object
 * @param id            clock id
 *
 * @return PEPPER_TRUE on success, otherwise PEPPER_FALSE
 *
 * It is recommended to use the same clock id with output backend. If multiple output backends use
 * different timer, you should not use pepper_compositor_get_time(). Also, be aware that changing
 * clock id might cause the timer base changes.
 */
PEPPER_API pepper_bool_t
pepper_compositor_set_clock_id(pepper_compositor_t *compositor, clockid_t id)
{
    struct timespec ts;

    if (compositor->clock_used)
    {
        if (compositor->clock_id == id)
            return PEPPER_TRUE;
    }

    if (clock_gettime(id, &ts) < 0)
        return PEPPER_FALSE;

    compositor->clock_id = id;
    compositor->clock_used = PEPPER_TRUE;

    return PEPPER_TRUE;
}

/**
 * Get current time of the given compositor with undefined base
 *
 * @param compositor    compositor object
 * @param ts            pointer to receive the current time
 *
 * @return PEPPER_TRUE on success, otherwise PEPPER_FALSE
 */
PEPPER_API pepper_bool_t
pepper_compositor_get_time(pepper_compositor_t *compositor, struct timespec *ts)
{
    if (clock_gettime(compositor->clock_id, ts) < 0)
        return PEPPER_FALSE;

    compositor->clock_used = PEPPER_TRUE;
    return PEPPER_TRUE;
}
