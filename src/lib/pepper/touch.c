#include "pepper-internal.h"

static void
touch_release(struct wl_client *client, struct wl_resource *resource)
{
    wl_resource_destroy(resource);
}

static const struct wl_touch_interface touch_impl =
{
    touch_release,
};

pepper_touch_t *
pepper_touch_create(pepper_seat_t *seat)
{
    pepper_touch_t *touch =
        (pepper_touch_t *)pepper_object_alloc(PEPPER_OBJECT_TOUCH, sizeof(pepper_touch_t));

    PEPPER_CHECK(touch, return NULL, "pepper_object_alloc() failed.\n");

    pepper_input_init(&touch->input, seat);
    return touch;
}

void
pepper_touch_destroy(pepper_touch_t *touch)
{
    pepper_input_fini(&touch->input);
    free(touch);
}

void
pepper_touch_bind_resource(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
    pepper_seat_t *seat = (pepper_seat_t *)wl_resource_get_user_data(resource);

    if (seat->touch)
        return;

    pepper_input_bind_resource(&seat->touch->input, client, wl_resource_get_version(resource),
                               id, &wl_touch_interface, &touch_impl, seat->touch);
}

PEPPER_API void
pepper_touch_set_focus(pepper_touch_t *touch, pepper_view_t *view)
{
    /* TODO: */
}

PEPPER_API pepper_view_t *
pepper_touch_get_focus(pepper_touch_t *touch)
{
    /* TODO: */
    return NULL;
}
