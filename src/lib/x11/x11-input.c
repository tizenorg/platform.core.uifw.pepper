#include "x11-internal.h"
#include <stdlib.h>

#define UNUSED(x)   (void)(x)

void
x11_handle_input_event(x11_seat_t* seat, uint32_t type, xcb_generic_event_t* xev)
{
    switch (type)
    {
    case XCB_ENTER_NOTIFY:
        {
            PEPPER_TRACE("enter\n");
        }
        break;
    case XCB_LEAVE_NOTIFY:
        {
            PEPPER_TRACE("leave\n");
        }
        break;
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE:
        break;
    case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t      *bp = (xcb_button_press_event_t *)xev;
            pepper_pointer_button_event_t  event;

            switch (bp->detail)
            {
            case XCB_BUTTON_INDEX_1:/* FIXME: LEFT */
            case XCB_BUTTON_INDEX_3:/* FIXME: RIGHT */
                {
                }
                break;
            case XCB_BUTTON_INDEX_4:
                /* TODO: axis up */
                break;
            case XCB_BUTTON_INDEX_5:
                /* TODO: axis down */
                break;
            default:
                PEPPER_TRACE("wheel or something pressed\n");
                break;
            }

            event.time   = bp->time;
            event.button = bp->detail;
            event.state  = WL_POINTER_BUTTON_STATE_PRESSED; /* FIXME */

            pepper_object_emit_event((pepper_object_t *)seat->pointer,
                                     PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON, &event);
        }
        break;
    case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t    *br = (xcb_button_release_event_t *)xev;
            pepper_pointer_button_event_t  event;

            switch (br->detail)
            {
            case XCB_BUTTON_INDEX_1:/* FIXME: LEFT */
                PEPPER_TRACE("left released\n");
                break;
            case XCB_BUTTON_INDEX_3:/* FIXME: RIGHT */
                PEPPER_TRACE("right released\n");
                break;
            default:
                PEPPER_TRACE("wheel or something pressed\n");
                break;
            }

            event.time   = br->time;
            event.button = br->detail;
            event.state  = WL_POINTER_BUTTON_STATE_RELEASED; /* FIXME */

            pepper_object_emit_event((pepper_object_t *)seat->pointer,
                                     PEPPER_EVENT_INPUT_DEVICE_POINTER_BUTTON, &event);
        }
        break;
    case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t     *motion = (xcb_motion_notify_event_t *)xev;
            pepper_pointer_motion_event_t  event;

            event.time = motion->time;
            event.x = motion->event_x;
            event.y = motion->event_y;

            pepper_object_emit_event((pepper_object_t *)seat->pointer,
                                     PEPPER_EVENT_INPUT_DEVICE_POINTER_MOTION_ABSOLUTE, &event);
        }
        break;
    default :
        PEPPER_ERROR("unknown input event, [0x%x]\n", type);
    }
}

void
x11_window_input_property_change(xcb_connection_t *conn, xcb_window_t window)
{
     static const uint32_t values[] =
     {
         XCB_EVENT_MASK_EXPOSURE |
         XCB_EVENT_MASK_STRUCTURE_NOTIFY |
         XCB_EVENT_MASK_KEY_PRESS |
         XCB_EVENT_MASK_KEY_RELEASE |
         XCB_EVENT_MASK_BUTTON_PRESS |
         XCB_EVENT_MASK_BUTTON_RELEASE |
         XCB_EVENT_MASK_POINTER_MOTION |
         XCB_EVENT_MASK_ENTER_WINDOW |
         XCB_EVENT_MASK_LEAVE_WINDOW |
         XCB_EVENT_MASK_KEYMAP_STATE |
         XCB_EVENT_MASK_FOCUS_CHANGE
     };
     xcb_change_window_attributes(conn, window, XCB_CW_EVENT_MASK, values);
     xcb_flush(conn);
}

static void
x11_seat_destroy(void *data)
{
    x11_seat_t *seat = (x11_seat_t *)data;

    if (seat->pointer)
        pepper_input_device_destroy(seat->pointer);

    if (seat->keyboard)
        pepper_input_device_destroy(seat->keyboard);

    if (seat->conn)
        seat->conn->seat = NULL;

    free(seat);
}

static void
handle_connection_destroy(struct wl_listener *listener, void *data)
{
    x11_seat_t *seat = wl_container_of(listener, seat, conn_destroy_listener);
    x11_seat_destroy(seat);
}

PEPPER_API void
pepper_x11_seat_create(pepper_x11_connection_t* conn)
{
    x11_seat_t      *seat;
    x11_output_t    *out, *tmp;

    if (!conn)
    {
        PEPPER_ERROR("connection is null...\n");
        return ;
    }

    seat = calloc(1, sizeof(x11_seat_t));
    if (!seat)
    {
        PEPPER_ERROR("failed to allocate memory\n");
        return ;
    }

    conn->use_xinput = PEPPER_TRUE;

    wl_list_for_each_safe(out, tmp, &conn->outputs, link)
        x11_window_input_property_change(conn->xcb_connection, out->window);

    /* XXX: if x-input-module used without x-output-module,
     * need to create dummy window for input with output-size */

    seat->conn_destroy_listener.notify = handle_connection_destroy;
    wl_signal_add(&conn->destroy_signal, &seat->conn_destroy_listener);

    seat->id = X11_BACKEND_INPUT_ID;

    /* Hard-coded: */
    seat->pointer = pepper_input_device_create(conn->compositor, WL_SEAT_CAPABILITY_POINTER,
                                               NULL, NULL);
    if (!seat->pointer)
    {
        PEPPER_ERROR("failed to create pepper pointer device\n");

        x11_seat_destroy(seat);
        return ;
    }
    seat->caps |= WL_SEAT_CAPABILITY_POINTER;

    seat->keyboard = pepper_input_device_create(conn->compositor, WL_SEAT_CAPABILITY_KEYBOARD,
                                               NULL, NULL);
    if (!seat->keyboard)
    {
        PEPPER_ERROR("failed to create pepper keyboard device\n");

        x11_seat_destroy(seat);
        return ;
    }
    seat->caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    /* x-connection has only 1 seat */
    conn->seat = seat;
    seat->conn = conn;
}
