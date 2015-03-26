#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include "common.h"
#include "pepper.h"
#include <wayland-util.h>
#include <pixman.h>

typedef struct pepper_surface   pepper_surface_t;
typedef struct pepper_region    pepper_region_t;

/* compositor */
struct pepper_compositor
{
    char                    *socket_name;
    struct wl_display       *display;
    struct wl_list          surfaces;
    struct wl_list          regions;
};

struct pepper_output
{
    pepper_compositor_t *compositor;

    struct wl_global    *global;
    struct wl_list      resources;

    pepper_output_geometry_t    geometry;
    int32_t                     scale;

    int                     mode_count;
    pepper_output_mode_t    *modes;
    pepper_output_mode_t    *current_mode;

    /* Backend-specific variables. */
    pepper_output_interface_t   interface;
    void                        *data;
};

struct pepper_surface
{
    struct wl_resource  *resource;
};

struct pepper_region
{
    pepper_compositor_t *compositor;
    struct wl_resource  *resource;
    pixman_region32_t   pixman_region;
};

pepper_surface_t *
pepper_surface_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id);

pepper_region_t *
pepper_region_create(pepper_compositor_t *compositor,
                      struct wl_client *client,
                      struct wl_resource *resource,
                      uint32_t id);

void
pepper_region_destroy(pepper_region_t *region);

#endif /* PEPPER_INTERNAL_H */
