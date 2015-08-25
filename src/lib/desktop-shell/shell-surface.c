#include "desktop-shell-internal.h"
#include "xdg-shell-server-protocol.h"


extern shell_pointer_grab_interface_t shell_pointer_move_grab;
extern shell_pointer_grab_interface_t shell_pointer_resize_grab;

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
shsurf_stop_listen_commit_event(shell_surface_t *shsurf)
{
    pepper_event_listener_remove(shsurf->surface_commit_listener);
}

static void
handle_surface_commit(pepper_event_listener_t *listener,
                      pepper_object_t *surface, uint32_t id, void *info, void *data)
{
    shell_surface_t *shsurf = data;

    if (!shsurf->mapped && shsurf->ack_configure && shsurf->shell_surface_map)
        shsurf->shell_surface_map(shsurf);
}

static void
shsurf_start_listen_commit_event(shell_surface_t *shsurf)
{
    shsurf->surface_commit_listener =
        pepper_object_add_event_listener((pepper_object_t *)shsurf->surface,
                                         PEPPER_EVENT_SURFACE_COMMIT, 0,
                                         handle_surface_commit, shsurf);
}

static void
handle_surface_destroy(pepper_event_listener_t *listener,
                       pepper_object_t *surface, uint32_t id, void *info, void *data)
{
    shell_surface_t *shsurf = data;
    shell_surface_t *child, *tmp;

    pepper_event_listener_remove(shsurf->surface_destroy_listener);
    shsurf_stop_listen_commit_event(shsurf);

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

    if (shsurf->fullscreen.background_surface)
    {
        /* TODO: pepper_surface_destroy(shsurf->fullscreen.background_surface); */;
    }

    if (shsurf->fullscreen.background_view)
        pepper_view_destroy(shsurf->fullscreen.background_view);

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

    /* TODO: XDG_SURFACE_STATE_ACTIVATED */

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

    shsurf->view    = pepper_compositor_add_surface_view(shell_client->shell->compositor, surface);
    if (!shsurf->view)
    {
        PEPPER_ERROR("pepper_compositor_add_view failed\n");
        goto error;
    }

    pepper_list_init(&shsurf->child_list);
    pepper_list_init(&shsurf->parent_link);
    pepper_list_insert(&shsurf->shell->shell_surface_list, &shsurf->link);

    wl_resource_set_implementation(shsurf->resource, implementation, shsurf, handle_resource_destroy);

    shsurf->surface_destroy_listener =
        pepper_object_add_event_listener((pepper_object_t *)surface, PEPPER_EVENT_OBJECT_DESTROY, 0,
                                         handle_surface_destroy, shsurf);

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_NONE);
    shsurf_start_listen_commit_event(shsurf);
    set_shsurf_to_surface(surface, shsurf);

    role = pepper_surface_get_role(shsurf->surface);

    if (!strcmp(role, "wl_shell_surface"))
        shsurf->send_configure = shsurf_wl_shell_surface_send_configure;
    else if (!strcmp(role, "xdg_surface"))
        shsurf->send_configure = shsurf_xdg_surface_send_configure;
    else if (!strcmp(role, "xdg_popup"))
        shsurf->send_configure = shsurf_xdg_popup_send_configure;

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

/* XXX: Temporary code for test, to be deleted */
static void
shell_surface_map_toplevel(shell_surface_t *shsurf);

void
shell_surface_set_toplevel(shell_surface_t *shsurf)
{
    shell_surface_set_parent(shsurf, NULL);

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_TOPLEVEL);

    /* Need to map in later */
    /* XXX: Temporary code for test, to be deleted */
    shell_surface_map_toplevel(shsurf);
}

void
shell_surface_set_popup(shell_surface_t     *shsurf,
                        pepper_seat_t       *seat,
                        pepper_surface_t    *parent,
                        int32_t              x,
                        int32_t              y,
                        uint32_t             flags)
{
    shell_surface_set_parent(shsurf, parent);

    shsurf->popup.x     = x;
    shsurf->popup.y     = y;
    shsurf->popup.flags = flags;
    shsurf->popup.seat  = seat;

    shell_surface_set_type(shsurf, SHELL_SURFACE_TYPE_POPUP);
}

void
shell_surface_set_transient(shell_surface_t     *shsurf,
                            pepper_surface_t    *parent,
                            int32_t              x,
                            int32_t              y,
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
    pepper_output_t *output = NULL;

    /* TODO: Find the output on which the surface has the biggest surface area */

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
    pixman_rectangle32_t area;

    /* XXX: If the given shell_surface has a parent, what's the corrent behavior? */
    shell_surface_set_parent(shsurf, NULL);

    /*  If the client does not specify the output then the compositor will apply its policy */
    if (!output)
        output = shell_surface_get_output(shsurf);

    shsurf->maximized.output = output;

    /* Save current position and size for unset_maximized */
    shell_surface_get_geometry(shsurf, &area);
    shsurf->saved.x = area.x;
    shsurf->saved.y = area.y;
    shsurf->saved.w = area.width;
    shsurf->saved.h = area.height;

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
    pixman_rectangle32_t                 area;

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
    shell_surface_get_geometry(shsurf, &area);
    shsurf->saved.x = area.x;
    shsurf->saved.y = area.y;
    shsurf->saved.w = area.width;
    shsurf->saved.h = area.height;

    geom = pepper_output_get_geometry(output);

    /* Send configure */
    shsurf->send_configure(shsurf, geom->w, geom->h);
}

void
shell_surface_unset_fullscreen(shell_surface_t *shsurf)
{
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
shell_surface_set_position(shell_surface_t *shsurf, int32_t x, int32_t y)
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
shell_surface_set_geometry(shell_surface_t *shsurf, int32_t x, int32_t y, int32_t w, int32_t h)
{
    pepper_view_set_position(shsurf->view, (double)x, (double)y);
    pepper_view_resize(shsurf->view, w, h);
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

static void
shell_surface_map_toplevel(shell_surface_t *shsurf)
{
    int32_t x = 0, y = 0;

    /* Restore original geometry */
    if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN ||
        shsurf->type == SHELL_SURFACE_TYPE_MAXIMIZED  ||
        shsurf->type == SHELL_SURFACE_TYPE_MINIMIZED)
    {
        x = shsurf->saved.x;
        y = shsurf->saved.y;

        pepper_view_resize(shsurf->view, shsurf->saved.w, shsurf->saved.h);

        if (shsurf->type == SHELL_SURFACE_TYPE_FULLSCREEN)
        {
            /* TODO: pepper_surface_destroy(shsurf->fullscreen.background_surface); */
            pepper_view_destroy(shsurf->fullscreen.background_view);

            shsurf->fullscreen.background_surface = NULL;
            shsurf->fullscreen.background_view    = NULL;
        }
    }
    else
    {
        /**
         * TODO: To get view's initial position, need to know output's size, position
         *       seat->pointer's position, or read from config file
         */
    }

    shell_surface_set_position(shsurf, x, y);

    pepper_view_map(shsurf->view);

    shsurf->mapped    = PEPPER_TRUE;
    shsurf->type      = SHELL_SURFACE_TYPE_TOPLEVEL;
    shsurf->next_type = SHELL_SURFACE_TYPE_NONE;
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

    /* TODO: add_popup_grab(), but how? */

    shsurf->mapped    = PEPPER_TRUE;
    shsurf->type      = SHELL_SURFACE_TYPE_POPUP;
    shsurf->next_type = SHELL_SURFACE_TYPE_NONE;
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

    shsurf->mapped    = PEPPER_TRUE;
    shsurf->type      = SHELL_SURFACE_TYPE_TRANSIENT;
    shsurf->next_type = SHELL_SURFACE_TYPE_NONE;
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

    shsurf->mapped    = PEPPER_TRUE;
    shsurf->type      = SHELL_SURFACE_TYPE_MINIMIZED;
    shsurf->next_type = SHELL_SURFACE_TYPE_NONE;
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
    float x, y;

    x = output->x + (output->w - surface_geom->width  * scale) / 2 - surface_geom->x;
    y = output->y + (output->h - surface_geom->height * scale) / 2 - surface_geom->y;

    shell_surface_set_position(shsurf, x, y);
}

static void
switch_output_mode(pepper_output_t *output, pepper_output_mode_t *mode)
{
    pepper_output_mode_t *new_mode = NULL;;

    /* TODO: Find the output mode to the smallest mode that can fit */

    pepper_output_set_mode(output, new_mode);
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

    /* TODO: Create background black view */
    {
        pepper_surface_t *surface = NULL;

        /**
         * surface->x = output_geom->w;
         * surface->y = output_geom->y;
         */

        shsurf->fullscreen.background_surface = surface;
        shsurf->fullscreen.background_view =
            pepper_compositor_add_surface_view(shsurf->shell->compositor, surface);
    }

    /* Place background black view */
    pepper_view_set_position(shsurf->fullscreen.background_view, output_geom->x, output_geom->y);
    pepper_view_map(shsurf->fullscreen.background_view);
    pepper_view_stack_top(shsurf->fullscreen.background_view, PEPPER_TRUE /*FIXME*/ );

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

void
shell_surface_move(shell_surface_t *shsurf, pepper_seat_t *seat, uint32_t serial)
{
    shell_seat_t *shseat = pepper_object_get_user_data((pepper_object_t *)seat, shsurf->shell);

    double x, y;

    pepper_view_get_position(shsurf->view, &x, &y);
    shsurf->move.vx = (int)x;
    shsurf->move.vy = (int)y;

    pepper_pointer_get_position(shseat->pointer_grab.pointer, &shsurf->move.px, &shsurf->move.py);

    shell_seat_pointer_start_grab(shseat, &shell_pointer_move_grab, shsurf);
}

void
shell_surface_resize(shell_surface_t *shsurf, pepper_seat_t *seat, uint32_t serial, uint32_t edges)
{
    double x, y;
    shell_seat_t *shseat = pepper_object_get_user_data((pepper_object_t *)seat, shsurf->shell);

    pepper_view_get_position(shsurf->view, &x, &y);
    pepper_pointer_get_position(shseat->pointer_grab.pointer,
                                &shsurf->resize.px,
                                &shsurf->resize.py);

    shsurf->resize.vx = (int)x;
    shsurf->resize.vy = (int)y;

    pepper_view_get_size(shsurf->view, &shsurf->resize.vw, &shsurf->resize.vh);

    shsurf->resize.edges = edges;
    shsurf->resize.resizing = PEPPER_TRUE;

    if (shsurf_is_xdg_surface(shsurf))
    {
        /* FIXME */
        shsurf->send_configure(shsurf, 0, 0);
    }

    shell_seat_pointer_start_grab(shseat, &shell_pointer_resize_grab, shsurf);
}
