#include "desktop-shell-internal.h"
#include "xdg-shell-server-protocol.h"
#include <linux/input.h>

void
remove_ping_timer(shell_client_t *shell_client)
{
    if (shell_client->ping_timer)
    {
        wl_event_source_remove(shell_client->ping_timer);
        shell_client->ping_timer = NULL;
    }
}

static void
handle_surface_commit(pepper_event_listener_t *listener,
                      pepper_object_t *surface, uint32_t id, void *info, void *data)
{
    shell_surface_t *shsurf = data;

    int     vw, vh;
    double  sx, sy;

    if (shsurf->has_next_geometry)
    {
        shsurf->geometry = shsurf->next_geometry;
        shsurf->has_next_geometry = PEPPER_FALSE;
    }

    if (!shsurf->mapped && shsurf->ack_configure && shsurf->shell_surface_map)
    {
        shsurf->shell_surface_map(shsurf);

        shsurf->mapped    = PEPPER_TRUE;
        shsurf->type      = shsurf->next_type;
        shsurf->next_type = SHELL_SURFACE_TYPE_NONE;
    }

    pepper_view_get_size(shsurf->view, &vw, &vh);

    sx = (vw - shsurf->last_width);
    sy = (vh - shsurf->last_height);

    if (sx || sy)
    {
        double vx, vy;

        pepper_view_get_position(shsurf->view, &vx, &vy);

        if (shsurf->resize.edges & WL_SHELL_SURFACE_RESIZE_LEFT)
            vx = vx - sx;

        if (shsurf->resize.edges & WL_SHELL_SURFACE_RESIZE_TOP)
            vy = vy - sy;

        pepper_view_set_position(shsurf->view, vx, vy);

        shsurf->last_width = vw;
        shsurf->last_height = vh;
    }
}

static void
shell_surface_end_popup_grab(shell_surface_t *shsurf);

static void
handle_surface_destroy(pepper_event_listener_t *listener,
                       pepper_object_t *surface, uint32_t id, void *info, void *data)
{
    shell_surface_t *shsurf = data;
    shell_surface_t *child, *tmp;

    if (shsurf->type == SHELL_SURFACE_TYPE_POPUP)
    {
        shell_surface_end_popup_grab(shsurf);
    }

    pepper_event_listener_remove(shsurf->surface_destroy_listener);
    pepper_event_listener_remove(shsurf->surface_commit_listener);

    pepper_event_listener_remove(shsurf->focus_enter_listener);
    pepper_event_listener_remove(shsurf->focus_leave_listener);

    if (shsurf->resource)
        wl_resource_destroy(shsurf->resource);

    if (shsurf->title)
        free(shsurf->title);

    if (shsurf->class_)
        free(shsurf->class_);

    pepper_list_for_each_safe(child, tmp, &shsurf->child_list, parent_link)
        shell_surface_set_parent(child, NULL);

    pepper_list_remove(&shsurf->parent_link);
    pepper_list_remove(&shsurf->link);

    free(shsurf);
}

static void
handle_resource_destroy(struct wl_resource *resource)
{
    shell_surface_t *shsurf = wl_resource_get_user_data(resource);

    shsurf->resource = NULL;
}

static void
shsurf_wl_shell_surface_send_configure(shell_surface_t *shsurf, int32_t width, int32_t height)
{
    uint32_t edges = 0; /* FIXME */

    wl_shell_surface_send_configure(shsurf->resource,
                                    edges,
                                    width,
                                    height);

    /* wl_shell_surface dont need this */
    shsurf->ack_configure = PEPPER_TRUE;
}

static void
shsurf_xdg_surface_send_configure(shell_surface_t *shsurf, int32_t width, int32_t height)
{
    struct wl_display   *display;
    struct wl_array      states;
    uint32_t            *state;
    uint32_t             serial;

    wl_array_init(&states);

    if (shsurf->next_type == SHELL_SURFACE_TYPE_MAXIMIZED)
    {
        state  = wl_array_add(&states, sizeof(uint32_t));
        *state = XDG_SURFACE_STATE_MAXIMIZED;
    }
    else if (shsurf->next_type == SHELL_SURFACE_TYPE_FULLSCREEN)
    {
        state  = wl_array_add(&states, sizeof(uint32_t));
        *state = XDG_SURFACE_STATE_FULLSCREEN;
    }

    if (shsurf->resize.resizing )
    {
        state  = wl_array_add(&states, sizeof(uint32_t));
        *state = XDG_SURFACE_STATE_RESIZING;
    }

    if (shsurf->has_keyboard_focus)
    {
        state  = wl_array_add(&states, sizeof(uint32_t));
        *state = XDG_SURFACE_STATE_ACTIVATED;
    }

    /* Send configure event */
    display = pepper_compositor_get_display(shsurf->shell->compositor);
    serial  = wl_display_next_serial(display);

    xdg_surface_send_configure(shsurf->resource,
                               width,
                               height,
                               &states,
                               serial);

    wl_array_release(&states);

    shsurf->ack_configure = PEPPER_FALSE;
}

static void
shsurf_xdg_popup_send_configure(shell_surface_t *shsurf, int32_t width, int32_t height)
{
    /* Do nothing */
}

static pepper_bool_t
shsurf_is_wl_shell_surface(shell_surface_t *shsurf)
{
    return (shsurf->send_configure == shsurf_wl_shell_surface_send_configure);
}

static pepper_bool_t
shsurf_is_xdg_surface(shell_surface_t *shsurf)
{
    return (shsurf->send_configure == shsurf_xdg_surface_send_configure);
}

static pepper_bool_t
shsurf_is_xdg_popup(shell_surface_t *shsurf)
{
    return (shsurf->send_configure == shsurf_xdg_popup_send_configure);
}

static void
shell_surface_focus_enter(shell_surface_t *shsurf)
{
    shsurf->has_keyboard_focus = PEPPER_TRUE;
}

static void
shell_surface_focus_leave(shell_surface_t *shsurf)
{
    shsurf->has_keyboard_focus = PEPPER_FALSE;
}

static void
shell_surface_handle_pointer_focus(shell_surface_t *shsurf, uint32_t id)
{
    if (id == PEPPER_EVENT_FOCUS_ENTER)
        shell_surface_ping(shsurf);
}

static void
shell_surface_handle_keyboard_focus(shell_surface_t *shsurf, uint32_t id)
{
    if (id == PEPPER_EVENT_FOCUS_ENTER)
        shell_surface_focus_enter(shsurf);
    else
        shell_surface_focus_leave(shsurf);

    /* Send state changed configure */
    shsurf->send_configure(shsurf, 0, 0);
}

static void
handle_focus(pepper_event_listener_t *listener,
             pepper_object_t *view, uint32_t id, void *info, void *data)
{
    pepper_object_type_t  type   = pepper_object_get_type((pepper_object_t *)info);

    switch (type)
    {
    case PEPPER_OBJECT_POINTER:
        shell_surface_handle_pointer_focus(data, id);
        break;
    case PEPPER_OBJECT_KEYBOARD:
        shell_surface_handle_keyboard_focus(data, id);
        break;
    case PEPPER_OBJECT_TOUCH:
        /* TODO */
        break;
    default:
        break;
    }
}

shell_surface_t *
shell_surface_create(shell_client_t *shell_client, pepper_surface_t *surface,
                     struct wl_client *client, const struct wl_interface *interface,
                     const void *implementation, uint32_t version, uint32_t id)
{
    shell_surface_t     *shsurf = NULL;
    const char          *role = NULL;

    shsurf = calloc(1, sizeof(shell_surface_t));
    if (!shsurf)
    {
        PEPPER_ERROR("Memory allocation faiiled\n");
        goto error;
    }

    shsurf->resource = wl_resource_create(client, interface, version, id);
    if (!shsurf->resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        goto error;
    }

    shsurf->shell_client = shell_client;
    shsurf->shell        = shell_client->shell;
    shsurf->client       = client;
    shsurf->surface      = surface;

    shsurf->view    = pepper_compositor_add_view(shell_client->shell->compositor);
    if (!shsurf->view)
    {
        PEPPER_ERROR("pepper_compositor_add_view failed\n");
        goto error;
    }

    if (!pepper_view_set_surface(shsurf->view, surface))
    {
        PEPPER_ERROR("pepper_view_set_surface() failed.\n");
        goto error;
    }

    pepper_list_init(&shsurf->child_list);
    pepper_list_init(&shsurf->parent_link);
    pepper_list_insert(&shsurf->shell->shell_surface_list, &shsurf->link);

    wl_resource_set_implementation(shsurf->resource, implementation, shsurf, handle_resource_destroy);

    shsurf->surface_destroy_listener =
        pepper_object_add_event_listener((pepper_object_t *)surface, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                         handle_surface_destroy, shsurf);

    shsurf->surface_commit_listener =
        pepper_object_add_event_listener((pepper_object_t *)surface, PEPPER_EVENT_SURFACE_COMMIT, 0,
                                         handle_surface_commit, shsurf);

    shsurf->focus_enter_listener =
        pepper_object_add_event_listener((pepper_object_t *)shsurf->view, PEPPER_EVENT_FOCUS_ENTER, 0,
                                         handle_focus, shsurf);

    shsurf->focus_leave_listener =
        pepper_object_add_event_listener((pepper_object_t *)shsurf->view, PEPPER_EVENT_FOCUS_LEAVE, 0,
                                         handle_focus, shsurf);


    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_NONE);
    set_shsurf_to_surface(surface, shsurf);

    role = pepper_surface_get_role(shsurf->surface);

    if (!strcmp(role, "wl_shell_surface"))
        shsurf->send_configure = shsurf_wl_shell_surface_send_configure;
    else if (!strcmp(role, "xdg_surface"))
        shsurf->send_configure = shsurf_xdg_surface_send_configure;
    else if (!strcmp(role, "xdg_popup"))
        shsurf->send_configure = shsurf_xdg_popup_send_configure;

    shsurf->ack_configure = PEPPER_TRUE;

    return shsurf;

error:
    if (shsurf)
        free(shsurf);

    return NULL;
}

static int
handle_ping_timeout(void *data)
{
    shell_client_t *shell_client = data;

    shell_client->irresponsive = PEPPER_TRUE;

    /* TODO: Display wait cursor */

    return 1;
}

void
shell_surface_ping(shell_surface_t *shsurf)
{
    shell_client_t      *shell_client = shsurf->shell_client;
    struct wl_display   *display;

    /* Already stucked, do not send another ping */
    if (shell_client->irresponsive)
    {
        handle_ping_timeout(shell_client);
        return ;
    }

    display = pepper_compositor_get_display(shsurf->shell->compositor);

    if (!shell_client->ping_timer)
    {
        struct wl_event_loop *loop = wl_display_get_event_loop(display);

        shell_client->ping_timer = wl_event_loop_add_timer(loop, handle_ping_timeout, shell_client);

        if (!shell_client->ping_timer)
        {
            PEPPER_ERROR("Failed to add timer event source.\n");
            return;
        }
    }

    wl_event_source_timer_update(shell_client->ping_timer, DESKTOP_SHELL_PING_TIMEOUT);

    shell_client->ping_serial = wl_display_next_serial(display);
    shell_client->need_pong   = PEPPER_TRUE;

    if (shsurf_is_wl_shell_surface(shsurf))
        wl_shell_surface_send_ping(shsurf->resource, shell_client->ping_serial);
    else if (shsurf_is_xdg_surface(shsurf) || shsurf_is_xdg_popup(shsurf))
        xdg_shell_send_ping(shell_client->resource, shell_client->ping_serial);
}

void
shell_client_handle_pong(shell_client_t *shell_client, uint32_t serial)
{
    /* Client response right ping_serial */
    if (shell_client->need_pong && shell_client->ping_serial == serial)
    {
        wl_event_source_timer_update(shell_client->ping_timer, 0);    /* disarms the timer */

        shell_client->irresponsive = PEPPER_FALSE;
        shell_client->need_pong    = PEPPER_FALSE;
        shell_client->ping_serial  = 0;

        /* TODO: Stop displaying wait cursor */
    }
}

void
shell_surface_handle_pong(shell_surface_t *shsurf, uint32_t serial)
{
    shell_client_handle_pong(shsurf->shell_client, serial);
}

static void
switch_output_mode(pepper_output_t *output, pepper_output_mode_t *mode)
{
    pepper_output_mode_t best_mode = {.w = (uint32_t)-1,.h = (uint32_t)-1,}, tmp_mode;
    int i, mode_count;

    /* TODO: Find the output mode to the smallest mode that can fit */

    mode_count = pepper_output_get_mode_count(output);

    for (i=0; i<mode_count; ++i)
    {
        pepper_output_get_mode(output, i, &tmp_mode);

        if (tmp_mode.w >= mode->w && tmp_mode.h >= mode->h)
        {
            if(best_mode.w > tmp_mode.w && best_mode.h > tmp_mode.h)
                best_mode = tmp_mode;
        }
    }

    if ( (uint32_t)best_mode.w != (uint32_t)-1)
        pepper_output_set_mode(output, &best_mode);
}

void
shell_surface_set_toplevel(shell_surface_t *shsurf)
{
    if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN)
    {
        if (shsurf->fullscreen.method == WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER)
            switch_output_mode(shsurf->fullscreen.output, &shsurf->saved.mode);
    }

    if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN ||
        shsurf->type == SHELL_SURFACE_TYPE_MAXIMIZED  ||
        shsurf->type == SHELL_SURFACE_TYPE_MINIMIZED)
    {
        shsurf->send_configure(shsurf, shsurf->saved.w, shsurf->saved.h);
    }

    shell_surface_set_parent(shsurf, NULL);

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_TOPLEVEL);
}

void
shell_surface_set_popup(shell_surface_t     *shsurf,
                        pepper_seat_t       *seat,
                        pepper_surface_t    *parent,
                        double               x,
                        double               y,
                        uint32_t             flags,
                        uint32_t             serial)
{
    shell_surface_set_parent(shsurf, parent);

    shsurf->popup.x      = x;
    shsurf->popup.y      = y;
    shsurf->popup.flags  = flags;
    shsurf->popup.seat   = seat;
    shsurf->popup.serial = serial;

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_POPUP);
}

void
shell_surface_set_transient(shell_surface_t     *shsurf,
                            pepper_surface_t    *parent,
                            double               x,
                            double               y,
                            uint32_t             flags)
{
    shell_surface_set_parent(shsurf, parent);

    shsurf->transient.x = x;
    shsurf->transient.y = y;
    shsurf->transient.flags = flags;

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_TRANSIENT);
}

static pepper_output_t *
shell_surface_get_output(shell_surface_t *shsurf)
{
    pepper_output_t        *output;
    const pepper_list_t    *list;
    pepper_list_t          *l;

    /* FIXME: Find the output on which the surface has the biggest surface area */
    list = pepper_compositor_get_output_list(shsurf->shell->compositor);
    pepper_list_for_each_list(l, list)
    {
        if (l->item)
        {
            output = (pepper_output_t *)l->item;
            break;
        }
    }

    return output;
}

static void
shell_surface_get_geometry(shell_surface_t *shsurf, pixman_rectangle32_t *geometry)
{
    double x, y;
    int    w, h;

    pepper_view_get_position(shsurf->view, &x, &y);
    pepper_view_get_size(shsurf->view, &w, &h);

    geometry->x      = (int32_t)x;
    geometry->y      = (int32_t)y;
    geometry->width  = (uint32_t)w;
    geometry->height = (uint32_t)h;
}

void
shell_surface_set_maximized(shell_surface_t     *shsurf,
                            pepper_output_t     *output)
{
    pixman_rectangle32_t    area;

    /* XXX: If the given shell_surface has a parent, what's the corrent behavior? */
    shell_surface_set_parent(shsurf, NULL);

    /*  If the client does not specify the output then the compositor will apply its policy */
    if (!output)
        output = shell_surface_get_output(shsurf);

    shsurf->maximized.output = output;

    /* Save current position and size for unset_maximized */
    pepper_view_get_position(shsurf->view, &shsurf->saved.x, &shsurf->saved.y);
    shsurf->saved.w = shsurf->geometry.w;
    shsurf->saved.h = shsurf->geometry.h;

    shell_get_output_workarea(shsurf->shell, output, &area);

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_MAXIMIZED);

    /* Send configure */
    shsurf->send_configure(shsurf, area.width, area.height);
}

void
shell_surface_unset_maximized(shell_surface_t *shsurf)
{
    /* TODO */
    shell_surface_set_toplevel(shsurf);
}

void
shell_surface_set_fullscreen(shell_surface_t    *shsurf,
                             pepper_output_t    *output,
                             uint32_t            method,
                             uint32_t            framerate)
{
    const pepper_output_geometry_t      *geom;

    /* XXX: If the given shell_surface has a parent, what's the corrent behavior? */
    shell_surface_set_parent(shsurf, NULL);

    /*  If the client does not specify the output then the compositor will apply its policy */
    if (!output)
        output = shell_surface_get_output(shsurf);

    shsurf->fullscreen.output    = output;
    shsurf->fullscreen.method    = method;
    shsurf->fullscreen.framerate = framerate;

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_FULLSCREEN);

    /* Save current position and size for unset_fullscreen */
    pepper_view_get_position(shsurf->view, &shsurf->saved.x, &shsurf->saved.y);
    shsurf->saved.w = shsurf->geometry.w;
    shsurf->saved.h = shsurf->geometry.h;

    /* Find current output mode */
    {
        pepper_output_mode_t mode;
        int i, mode_cnt;

        mode_cnt = pepper_output_get_mode_count(output);
        for (i=0; i<mode_cnt; ++i)
        {
            pepper_output_get_mode(output, i, &mode);
            if (mode.flags & PEPPER_OUTPUT_MODE_CURRENT)
            {
                shsurf->saved.mode = mode;
                break;
            }
        }
    }

    geom = pepper_output_get_geometry(output);

    /* Send configure */
    shsurf->send_configure(shsurf, geom->w, geom->h);
}

void
shell_surface_unset_fullscreen(shell_surface_t *shsurf)
{
    if (shsurf->fullscreen.method == WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER)
        switch_output_mode(shsurf->fullscreen.output, &shsurf->saved.mode);

    shell_surface_set_toplevel(shsurf);
}

void
shell_surface_set_minimized(shell_surface_t *shsurf)
{
    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_MINIMIZED);
}

void
shell_surface_ack_configure(shell_surface_t *shsurf, uint32_t serial)
{
    shsurf->ack_configure = PEPPER_TRUE;
}

static void
shell_surface_set_position(shell_surface_t *shsurf, double x, double y)
{
    pepper_view_set_position(shsurf->view, x, y);
}

shell_surface_t *
get_shsurf_from_surface(pepper_surface_t *surface, desktop_shell_t *shell)
{
    return pepper_object_get_user_data((pepper_object_t *)surface, shell);
}

void
set_shsurf_to_surface(pepper_surface_t *surface, shell_surface_t *shsurf)
{
    pepper_object_set_user_data((pepper_object_t *)surface, shsurf->shell, shsurf, NULL);
}

void
shell_surface_set_parent(shell_surface_t *shsurf, pepper_surface_t *parent)
{
    shell_surface_t *parent_shsurf;

    pepper_list_remove(&shsurf->parent_link);
    pepper_list_init(&shsurf->parent_link);

    shsurf->parent = parent;

    if (parent)
    {
        parent_shsurf = get_shsurf_from_surface(parent, shsurf->shell);

        if (parent_shsurf)
        {
            pepper_list_insert(&parent_shsurf->child_list, &shsurf->parent_link);
            pepper_view_set_parent(shsurf->view, parent_shsurf->view);
        }
    }
    else
    {
        pepper_view_set_parent(shsurf->view, NULL);
    }
}

void
shell_surface_set_geometry(shell_surface_t *shsurf, double x, double y, int32_t w, int32_t h)
{
    shsurf->next_geometry.x = x;
    shsurf->next_geometry.y = y;
    shsurf->next_geometry.w = w;
    shsurf->next_geometry.h = h;

    shsurf->has_next_geometry = PEPPER_TRUE;
}

pepper_bool_t
shell_surface_set_title(shell_surface_t *shsurf, const char* title)
{
    if (shsurf->title)
        free(shsurf->title);

    shsurf->title = strdup(title);

    if (!shsurf->title)
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

pepper_bool_t
shell_surface_set_class(shell_surface_t *shsurf, const char* class_)
{
    if (shsurf->class_)
        free(shsurf->class_);

    shsurf->class_ = strdup(class_);

    if (!shsurf->class_)
        return PEPPER_FALSE;

    return PEPPER_TRUE;
}

static pepper_output_t *
shell_pointer_get_output(desktop_shell_t *shell, pepper_pointer_t *pointer)
{
    pepper_output_t        *output = NULL;
    pepper_output_t        *tmp_output;
    const pepper_list_t    *list;
    pepper_list_t          *l;
    double                  px, py;

    pepper_pointer_get_position(pointer, &px, &py);

    list = pepper_compositor_get_output_list(shell->compositor);
    pepper_list_for_each_list(l, list)
    {
        tmp_output = (pepper_output_t *)l->item;

        if (tmp_output)
        {
            const pepper_output_geometry_t *geom;
            pixman_region32_t rect;

            if (!output)
                output = tmp_output;

            geom = pepper_output_get_geometry(tmp_output);

            pixman_region32_init_rect(&rect, geom->x, geom->y, geom->w, geom->h);

            if (pixman_region32_contains_point(&rect, px, py, NULL))
            {
                output = tmp_output;
                pixman_region32_fini(&rect);
                break;
            }

            pixman_region32_fini(&rect);
        }
    }

    return output;
}

static void
shell_surface_set_initial_position(shell_surface_t *shsurf)
{
    pepper_pointer_t    *pointer = NULL;
    shell_seat_t        *shseat, *tmp;
    double               vx = 0.f, vy = 0.f;

    pepper_list_for_each_safe(shseat, tmp, &shsurf->shell->shseat_list, link)
    {
        if (shseat->seat)
        {
            pointer = pepper_seat_get_pointer(shseat->seat);
            if (pointer)
                break;
        }
    }

    if (pointer)
    {
        const pepper_output_geometry_t  *geom;
        pepper_output_t                 *output;

        int32_t     vw, vh;
        double      px, py;

        pepper_pointer_get_position(pointer, &px, &py);

        output = shell_pointer_get_output(shsurf->shell, pointer);

        geom = pepper_output_get_geometry(output);

        pepper_view_get_size(shsurf->view, &vw, &vh);

        /* FIXME: consider output's origin is top-left */
        if ( px <= geom->x )
            vx = 0.f;
        else if ( px + vw > geom->x + geom->w)
            vx = geom->x + geom->w - vw;
        else
            vx = px;

        if ( py <= geom->y )
            vy = 0.f;
        else if ( py + vh > geom->y + geom->h )
            vy = geom->y + geom->h - vh;
        else
            vy = py;
    }

    pepper_view_set_position(shsurf->view, vx, vy);
}

static void
shell_surface_map_toplevel(shell_surface_t *shsurf)
{
    shell_seat_t        *shseat, *tmp;
    pepper_keyboard_t   *keyboard;

    if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN ||
        shsurf->type == SHELL_SURFACE_TYPE_MAXIMIZED  ||
        shsurf->type == SHELL_SURFACE_TYPE_MINIMIZED)
    {
        shell_surface_set_position(shsurf, shsurf->saved.x, shsurf->saved.y);
    }
    else
    {
        shell_surface_set_initial_position(shsurf);

        pepper_list_for_each_safe(shseat, tmp, &shsurf->shell->shseat_list, link)
        {
            if (shseat->seat)
            {
                keyboard = pepper_seat_get_keyboard(shseat->seat);
                if (keyboard)
                {
                    pepper_keyboard_set_focus(keyboard, shsurf->view);
                }
            }
        }
    }

    pepper_view_map(shsurf->view);
}

static void
pointer_popup_grab_motion(pepper_pointer_t *pointer, void *data,
                          uint32_t time, double x, double y)
{
    double               vx, vy;
    pepper_compositor_t *compositor = pepper_pointer_get_compositor(pointer);
    pepper_view_t       *view = pepper_compositor_pick_view(compositor, x, y, &vx, &vy);

    if (pepper_pointer_get_focus(pointer) != view)
    {
        pepper_pointer_send_leave(pointer);
        pepper_pointer_set_focus(pointer, view);
        pepper_pointer_send_enter(pointer, vx, vy);
    }

    pepper_pointer_send_motion(pointer, time, vx, vy);
}

static struct wl_client *
pepper_view_get_client(pepper_view_t *view)
{
    pepper_surface_t *surface = pepper_view_get_surface(view);
    if (surface)
    {
        struct wl_resource *resource = pepper_surface_get_resource(surface);
        if (resource)
            return wl_resource_get_client(resource);
    }

    return NULL;
}

static void
pointer_popup_grab_button(pepper_pointer_t *pointer, void *data,
                          uint32_t time, uint32_t button, uint32_t state)
{
    shell_surface_t     *shsurf = data;
    struct wl_client    *client = NULL;
    pepper_view_t       *focus;

    focus = pepper_pointer_get_focus(pointer);

    if (focus)
        client = pepper_view_get_client(focus);

    /* The popup_done event is sent out when a popup grab is broken, that is, when the user
     * clicks a surface that doesn't belong to the client owning the popup surface. */

    if (client == shsurf->client)
    {
        pepper_pointer_send_button(pointer, time, button, state);
    }
    else if (shsurf->popup.button_up)
    {
        shell_surface_end_popup_grab(data);
    }

    if (state == WL_POINTER_BUTTON_STATE_RELEASED)
        shsurf->popup.button_up = PEPPER_TRUE;
}

static void
pointer_popup_grab_axis(pepper_pointer_t *pointer, void *data,
                        uint32_t time, uint32_t axis, double value)
{
    /* TODO */
}

static void
pointer_popup_grab_cancel(pepper_pointer_t *pointer, void *data)
{
    shell_surface_end_popup_grab(data);
}

static pepper_pointer_grab_t pointer_popup_grab =
{
    pointer_popup_grab_motion,
    pointer_popup_grab_button,
    pointer_popup_grab_axis,
    pointer_popup_grab_cancel,
};

static void
shell_surface_send_popup_done(shell_surface_t *shsurf)
{
    if (shsurf->resource)
    {
        if (shsurf_is_xdg_popup(shsurf))
            xdg_popup_send_popup_done(shsurf->resource, shsurf->popup.serial);
        else if (shsurf_is_wl_shell_surface(shsurf) && shsurf->type == SHELL_SURFACE_TYPE_POPUP )
            wl_shell_surface_send_popup_done(shsurf->resource);
    }
}

static void
shell_surface_end_popup_grab(shell_surface_t *shsurf)
{
    pepper_pointer_t *pointer = pepper_seat_get_pointer(shsurf->popup.seat);

    if(pointer)
    {
        const pepper_pointer_grab_t *grab = pepper_pointer_get_grab(pointer);
        if (grab == &pointer_popup_grab )
            pepper_pointer_set_grab(pointer,
                                    shsurf->old_pointer_grab,
                                    shsurf->old_pointer_grab_data);
    }

    shell_surface_send_popup_done(shsurf);
}

static void
shell_surface_add_popup_grab(shell_surface_t *shsurf)
{
    pepper_pointer_t *pointer = pepper_seat_get_pointer(shsurf->popup.seat);

    /* TODO: Find the object that serial belongs to */

    if (pointer)
    {
        shsurf->old_pointer_grab      = pepper_pointer_get_grab(pointer);
        shsurf->old_pointer_grab_data = pepper_pointer_get_grab_data(pointer);

        pepper_pointer_set_grab(pointer, &pointer_popup_grab, shsurf);

        shsurf->popup.button_up = PEPPER_FALSE;
    }

    /* TODO: touch */
}

static void
shell_surface_map_popup(shell_surface_t *shsurf)
{
    shell_surface_t *parent = get_shsurf_from_surface(shsurf->parent, shsurf->shell);

    /* Set position as relatively */
    pepper_view_set_parent(shsurf->view, parent->view);
    shell_surface_set_position(shsurf, shsurf->popup.x, shsurf->popup.y);

    pepper_view_map(shsurf->view);

    pepper_view_stack_top(shsurf->view, PEPPER_TRUE);

    shell_surface_add_popup_grab(shsurf);
}

static void
shell_surface_map_transient(shell_surface_t *shsurf)
{
    shell_surface_t *parent = get_shsurf_from_surface(shsurf->parent, shsurf->shell);
    double x, y;

    pepper_view_get_position(parent->view, &x, &y);

    pepper_view_set_parent(shsurf->view, parent->view);

    shell_surface_set_position(shsurf,
                               x + shsurf->transient.x,
                               y + shsurf->transient.y);

    if (shsurf->transient.flags != WL_SHELL_SURFACE_TRANSIENT_INACTIVE)
    {
        /* TODO: set keyboard focus to view */
    }

    pepper_view_map(shsurf->view);
}

static void
shell_surface_map_maximized(shell_surface_t *shsurf)
{
    pixman_rectangle32_t area;

    shell_get_output_workarea(shsurf->shell, shsurf->maximized.output, &area);

    shell_surface_set_position(shsurf, area.x, area.y);

    pepper_view_map(shsurf->view);

    /* Set top of z-order */
    pepper_view_stack_top(shsurf->view, PEPPER_TRUE /*FIXME:*/);
}

static void
shell_surface_map_minimized(shell_surface_t *shsurf)
{
    /* TODO */
    pepper_view_unmap(shsurf->view);
}

static float
get_scale(float output_w, float output_h, float view_w, float view_h)
{
    float scale, output_aspect, view_aspect;

    output_aspect = output_w / output_h;

    view_aspect = view_w / view_h;

    if (output_aspect < view_aspect)
        scale = output_w / view_w;
    else
        scale = output_h / view_h;

    return scale;
}

static void
shell_surface_center_on_output_by_scale(shell_surface_t                 *shsurf,
                                        const pepper_output_geometry_t  *output,
                                        pixman_rectangle32_t            *surface_geom,
                                        float                            scale)
{
    double x, y;

    x = output->x + (output->w - surface_geom->width  * scale) / 2;
    y = output->y + (output->h - surface_geom->height * scale) / 2;

    shell_surface_set_position(shsurf, x, y);
}

static void
shell_surface_map_fullscreen(shell_surface_t *shsurf)
{
    pepper_output_t                     *output;
    const pepper_output_geometry_t      *output_geom;
    pixman_rectangle32_t                 shsurf_geom;
    float                                scale = 0.f;

    output      = shsurf->fullscreen.output;
    output_geom = pepper_output_get_geometry(output);

    /* Get current geometry */
    shell_surface_get_geometry(shsurf, &shsurf_geom);

    switch (shsurf->fullscreen.method)
    {
    case WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE:
        {
            scale = get_scale(output_geom->w, output_geom->h, shsurf_geom.width, shsurf_geom.height);
        }
        break;
    case WL_SHELL_SURFACE_FULLSCREEN_METHOD_DRIVER:
        {
            pepper_output_mode_t mode;
            int32_t              buffer_scale;

            buffer_scale = pepper_surface_get_buffer_scale(shsurf->surface);

            mode.w       = shsurf_geom.width  * buffer_scale;
            mode.h       = shsurf_geom.height * buffer_scale;
            mode.refresh = shsurf->fullscreen.framerate;

            switch_output_mode(output, &mode);

            /* Recalculate output geometry */
            output_geom = pepper_output_get_geometry(output);

            scale = 1.f;
        }
        break;
    case WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT:
    case WL_SHELL_SURFACE_FULLSCREEN_METHOD_FILL:
        {
            scale = 1.f;
        }
        break;
    default:
        PEPPER_ERROR("invalid method type = 0x%x\n", shsurf->fullscreen.method);
        break;
    }

    /* Place target view */
    shell_surface_center_on_output_by_scale(shsurf, output_geom, &shsurf_geom, scale);
    pepper_view_map(shsurf->view);
    pepper_view_stack_top(shsurf->view, PEPPER_TRUE /*FIXME*/ );
}

void
shell_surface_set_type(shell_surface_t *shsurf, shell_surface_type_t type)
{
    if (shsurf->type == type )
        return;

    if (shsurf->next_type == type)
        return ;

    switch (type)
    {
    case SHELL_SURFACE_TYPE_NONE:
        shsurf->shell_surface_map = NULL;
        break;
    case SHELL_SURFACE_TYPE_TOPLEVEL:
        shsurf->shell_surface_map = shell_surface_map_toplevel;
        break;
    case SHELL_SURFACE_TYPE_POPUP:
        shsurf->shell_surface_map = shell_surface_map_popup;
        break;
    case SHELL_SURFACE_TYPE_TRANSIENT:
        shsurf->shell_surface_map = shell_surface_map_transient;
        break;
    case SHELL_SURFACE_TYPE_MAXIMIZED:
        shsurf->shell_surface_map = shell_surface_map_maximized;
        break;
    case SHELL_SURFACE_TYPE_FULLSCREEN:
        shsurf->shell_surface_map = shell_surface_map_fullscreen;
        break;
    case SHELL_SURFACE_TYPE_MINIMIZED:
        shsurf->shell_surface_map = shell_surface_map_minimized;
        break;
    default :
        PEPPER_ERROR("invalid surface type = 0x%x\n", type);
        return ;
    }

    shsurf->next_type = type;
    shsurf->mapped    = PEPPER_FALSE;
}

static void
pointer_move_grab_motion(pepper_pointer_t *pointer, void *data, uint32_t time, double x, double y)
{
    shell_surface_t *shsurf = data;
    pepper_view_set_position(shsurf->view, shsurf->move.dx + x, shsurf->move.dy + y);
}

static void
pointer_move_grab_button(pepper_pointer_t *pointer, void *data,
                         uint32_t time, uint32_t button, uint32_t state)
{
    if (button == BTN_LEFT && state == PEPPER_BUTTON_STATE_RELEASED)
    {
        shell_surface_t *shsurf = data;
        pepper_pointer_set_grab(pointer, shsurf->old_pointer_grab, shsurf->old_pointer_grab_data);
    }
}

static void
pointer_move_grab_axis(pepper_pointer_t *pointer, void *data,
                       uint32_t time, uint32_t axis, double value)
{
    /* TODO */
}

static void
pointer_move_grab_cancel(pepper_pointer_t *pointer, void *data)
{
    /* TODO */
}

static pepper_pointer_grab_t pointer_move_grab =
{
    pointer_move_grab_motion,
    pointer_move_grab_button,
    pointer_move_grab_axis,
    pointer_move_grab_cancel,
};

void
shell_surface_move(shell_surface_t *shsurf, pepper_seat_t *seat, uint32_t serial)
{
    pepper_pointer_t   *pointer = pepper_seat_get_pointer(seat);
    double              vx, vy;
    double              px, py;

    if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN ||
        shsurf->type == SHELL_SURFACE_TYPE_MAXIMIZED  ||
        shsurf->type == SHELL_SURFACE_TYPE_MINIMIZED)
    {
        return ;
    }

    /* TODO: Touch driven move?? */
    if (!pointer)
        return;

    /* TODO: Should consider view transform. */
    pepper_view_get_position(shsurf->view, &vx, &vy);
    pepper_pointer_get_position(pointer, &px, &py);

    shsurf->move.dx = vx - px;
    shsurf->move.dy = vy - py;

    shsurf->old_pointer_grab = pepper_pointer_get_grab(pointer);
    shsurf->old_pointer_grab_data = pepper_pointer_get_grab_data(pointer);
    pepper_pointer_set_grab(pointer, &pointer_move_grab, shsurf);
}

static void
pointer_resize_grab_motion(pepper_pointer_t *pointer, void *data, uint32_t time, double x, double y)
{
    shell_surface_t    *shsurf = data;
    double              dx = 0.f, dy = 0.f;

    if (shsurf->resize.edges & WL_SHELL_SURFACE_RESIZE_LEFT)
    {
        dx = shsurf->resize.px - x;
    }
    else if (shsurf->resize.edges & WL_SHELL_SURFACE_RESIZE_RIGHT)
    {
        dx = x - shsurf->resize.px;
    }

    if (shsurf->resize.edges & WL_SHELL_SURFACE_RESIZE_TOP)
    {
        dy = shsurf->resize.py - y;
    }
    else if(shsurf->resize.edges & WL_SHELL_SURFACE_RESIZE_BOTTOM)
    {
        dy = y - shsurf->resize.py;
    }

    shsurf->send_configure(shsurf, shsurf->resize.vw + dx, shsurf->resize.vh + dy);
}

static void
pointer_resize_grab_button(pepper_pointer_t *pointer, void *data,
                           uint32_t time, uint32_t button, uint32_t state)
{
    if (button == BTN_LEFT && state == PEPPER_BUTTON_STATE_RELEASED)
    {
        shell_surface_t *shsurf = data;

        pepper_pointer_set_grab(pointer, shsurf->old_pointer_grab, shsurf->old_pointer_grab_data);
        shsurf->resize.resizing = PEPPER_FALSE;
        shsurf->resize.edges    = 0;
    }
}

static void
pointer_resize_grab_axis(pepper_pointer_t *pointer, void *data,
                         uint32_t time, uint32_t axis, double value)
{
    /* TODO */
}

static void
pointer_resize_grab_cancel(pepper_pointer_t *pointer, void *data)
{
    /* TODO */
}

static pepper_pointer_grab_t pointer_resize_grab =
{
    pointer_resize_grab_motion,
    pointer_resize_grab_button,
    pointer_resize_grab_axis,
    pointer_resize_grab_cancel,
};

void
shell_surface_resize(shell_surface_t *shsurf, pepper_seat_t *seat, uint32_t serial, uint32_t edges)
{
    pepper_pointer_t   *pointer = pepper_seat_get_pointer(seat);

    if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN ||
        shsurf->type == SHELL_SURFACE_TYPE_MAXIMIZED  ||
        shsurf->type == SHELL_SURFACE_TYPE_MINIMIZED)
    {
        return ;
    }

    pepper_pointer_get_position(pointer, &shsurf->resize.px, &shsurf->resize.py);

    pepper_view_get_position(shsurf->view, &shsurf->resize.vx, &shsurf->resize.vy);

    shsurf->resize.vw  = shsurf->geometry.w;
    shsurf->resize.vh  = shsurf->geometry.h;

    shsurf->resize.edges = edges;
    shsurf->resize.resizing = PEPPER_TRUE;

    if (shsurf_is_xdg_surface(shsurf))
    {
        /* FIXME */
        shsurf->send_configure(shsurf, 0, 0);
    }

    shsurf->old_pointer_grab = pepper_pointer_get_grab(pointer);
    shsurf->old_pointer_grab_data = pepper_pointer_get_grab_data(pointer);
    pepper_pointer_set_grab(pointer, &pointer_resize_grab, shsurf);
}
