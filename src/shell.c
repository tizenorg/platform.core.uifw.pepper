#include "pepper_internal.h"


/* shell surface interface */
static void
shell_surface_pong(struct wl_client   *client,
		   struct wl_resource *resource,
		   uint32_t serial)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
shell_surface_move(struct wl_client   *client,
		   struct wl_resource *resource,
		   struct wl_resource *seat,
		   uint32_t serial)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
shell_surface_resize(struct wl_client   *client,
		     struct wl_resource *resource,
		     struct wl_resource *seat,
		     uint32_t            serial,
		     uint32_t            edges)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}



static void
shell_surface_set_toplevel(struct wl_client   *client,
			   struct wl_resource *resource)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}



static void
shell_surface_set_transient(struct wl_client   *client,
			    struct wl_resource *resource,
			    struct wl_resource *parent_resource,
			    int                 x,
			    int                 y,
			    uint32_t            flags)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
shell_surface_set_fullscreen(struct wl_client   *client,
			     struct wl_resource *resource,
			     uint32_t            method,
			     uint32_t            framerate,
			     struct wl_resource *output_resource)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
shell_surface_set_popup(struct wl_client   *client,
			struct wl_resource *resource,
			struct wl_resource *seat_resource,
			uint32_t            serial,
			struct wl_resource *parent_resource,
			int32_t             x,
			int32_t             y,
			uint32_t            flags)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
shell_surface_set_maximized(struct wl_client   *client,
                            struct wl_resource *resource,
                            struct wl_resource *output_resource)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}


static void
shell_surface_set_title(struct wl_client   *client,
			struct wl_resource *resource,
			const char         *title)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}

static void
shell_surface_set_class(struct wl_client   *client,
			struct wl_resource *resource,
			const char         *class)
{
    pepper_shell_surface_t *shsurface = wl_resource_get_user_data(resource);

    PEPPER_TRACE("%s\n", __FUNCTION__);
}



static const struct wl_shell_surface_interface pepper_shell_surface_implementation =
{
    shell_surface_pong,
    shell_surface_move,
    shell_surface_resize,
    shell_surface_set_toplevel,
    shell_surface_set_transient,
    shell_surface_set_fullscreen,
    shell_surface_set_popup,
    shell_surface_set_maximized,
    shell_surface_set_title,
    shell_surface_set_class
};



/* shell interface */
static void
shell_get_shell_surface(struct wl_client   *client,
			       struct wl_resource *resource,
			       uint32_t            id,
			       struct wl_resource *surface_resource)
{
    pepper_compositor_t    *compositor = wl_resource_get_user_data(resource);
    pepper_surface_t       *surface    = wl_resource_get_user_data(surface_resource);
    pepper_shell_surface_t *shsurface;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    shsurface = (pepper_shell_surface_t *)calloc(1, sizeof(pepper_shell_surface_t));

    if (!shsurface)
    {
	PEPPER_ERROR("%s Shell surface memory allocation failed\n", __FUNCTION__);
	wl_client_post_no_memory(client);
	return ;
    }

    shsurface->resource = wl_resource_create(client, &wl_shell_surface_interface,
					     wl_resource_get_version(resource), id);
    if (!shsurface->resource)
    {
	PEPPER_ERROR("%s wl_resource_create failed\n", __FUNCTION__);
	free(shsurface);
	wl_client_post_no_memory(client);
	return ;
    }

    wl_resource_set_implementation(shsurface->resource, &pepper_shell_surface_implementation,
				   shsurface, NULL);

}


static const struct wl_shell_interface shell_implementation =
{
    shell_get_shell_surface
};


void
bind_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
    pepper_compositor_t *compositor = (pepper_compositor_t *)data;
    struct wl_resource  *resource;

    PEPPER_TRACE("%s\n", __FUNCTION__);

    resource = wl_resource_create(client, &wl_shell_interface, version, id);
    if (!resource)
    {
	PEPPER_ERROR("%s wl_resource_create failed\n", __FUNCTION__);

	wl_client_post_no_memory(client);
	return;
    }

    wl_resource_set_implementation(resource, &shell_implementation, compositor, NULL);
}


