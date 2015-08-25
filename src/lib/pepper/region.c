#include "pepper-internal.h"

static void
region_resource_destroy_handler(struct wl_resource *resource)
{
    pepper_region_t *region = wl_resource_get_user_data(resource);
    pepper_region_destroy(region);
}

static void
region_destroy(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static void
region_add(struct wl_client *client, struct wl_resource *resource,
           int32_t x, int32_t y, int32_t w, int32_t h)
{
    pepper_region_t *region = wl_resource_get_user_data(resource);
    pixman_region32_union_rect(&region->pixman_region, &region->pixman_region,
                               x, y, w, h);
}

static void
region_subtract(struct wl_client *client, struct wl_resource *resource,
                int32_t x, int32_t y, int32_t w, int32_t h)
{
    pepper_region_t    *region = wl_resource_get_user_data(resource);
    pixman_region32_t   rect;

    pixman_region32_init_rect(&rect, x, y, w, h);
    pixman_region32_subtract(&region->pixman_region, &region->pixman_region, &rect);
    pixman_region32_fini(&rect);
}

static const struct wl_region_interface region_implementation =
{
    region_destroy,
    region_add,
    region_subtract,
};

pepper_region_t *
pepper_region_create(pepper_compositor_t   *compositor,
                     struct wl_client      *client,
                     struct wl_resource    *resource,
                     uint32_t               id)
{
    pepper_region_t *region = pepper_calloc(1, sizeof(pepper_region_t));
    if (!region)
    {
        PEPPER_ERROR("Surface memory allocation failed\n");
        wl_resource_post_no_memory(resource);
        return NULL;
    }

    region->compositor = compositor;
    region->resource = wl_resource_create(client, &wl_region_interface, 1, id);

    if (!region->resource)
    {
        PEPPER_ERROR("wl_resource_create failed\n");
        pepper_free(region);
        wl_resource_post_no_memory(resource);
        pepper_free(region);
        return NULL;
    }

    wl_resource_set_implementation(region->resource, &region_implementation,
                                   region, region_resource_destroy_handler);

    pepper_list_insert(&compositor->region_list, &region->link);
    return region;
}

void
pepper_region_destroy(pepper_region_t *region)
{
    pixman_region32_fini(&region->pixman_region);
    pepper_list_remove(&region->link);
    pepper_free(region);
}

static inline void
add_bbox_point(double *box, int x, int y, const pepper_mat4_t *matrix)
{
    pepper_vec2_t v = { x, y };

    pepper_mat4_transform_vec2(matrix, &v);

    box[0] = PEPPER_MIN(box[0], v.x);
    box[1] = PEPPER_MIN(box[1], v.y);
    box[2] = PEPPER_MAX(box[2], v.x);
    box[3] = PEPPER_MAX(box[3], v.y);
}

static inline void
transform_bounding_box(pixman_box32_t *box, const pepper_mat4_t *matrix)
{
    double          b[4] = { HUGE_VAL, HUGE_VAL, -HUGE_VAL, -HUGE_VAL };

    add_bbox_point(b, box->x1, box->y1, matrix);
    add_bbox_point(b, box->x2, box->y1, matrix);
    add_bbox_point(b, box->x2, box->y2, matrix);
    add_bbox_point(b, box->x1, box->y2, matrix);

    box->x1 = floor(b[0]);
    box->y1 = floor(b[1]);
    box->x2 = ceil(b[2]);
    box->y2 = ceil(b[3]);
}

void
pepper_transform_pixman_region(pixman_region32_t *region, const pepper_mat4_t *matrix)

{
    pixman_region32_t   result;
    pixman_box32_t     *rects;
    int                 i, num_rects;

    pixman_region32_init(&result);
    rects = pixman_region32_rectangles(region, &num_rects);

    for (i = 0; i < num_rects; i++)
    {
        pixman_box32_t box = rects[i];

        transform_bounding_box(&box, matrix);
        pixman_region32_union_rect(&result, &result,
                                   box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
    }

    pixman_region32_copy(region, &result);
    pixman_region32_fini(&result);
}
