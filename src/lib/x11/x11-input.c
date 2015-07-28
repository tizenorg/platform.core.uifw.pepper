#include "x11-internal.h"
#include <stdlib.h>

#define UNUSED(x)   (void)(x)

void
x11_handle_input_event(x11_seat_t* seat, uint32_t type, xcb_generic_event_t* xev)
{
    pepper_input_event_t event = {0,};

    UNUSED(event); /* disable warning */

    switch (type)
    {
    case XCB_ENTER_NOTIFY:
        {
            PEPPER_TRACE("enter\n");
            break;
        }
    case XCB_LEAVE_NOTIFY:
        {
            PEPPER_TRACE("leave\n");
            break;
        }
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE:
        break;
    case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t *bp = (xcb_button_press_event_t *)xev;
            switch (bp->detail)
            {
            case XCB_BUTTON_INDEX_1:/* FIXME: LEFT */
                PEPPER_TRACE("left click\n");
                event.index = 1;
                break;
            case XCB_BUTTON_INDEX_3:/* FIXME: RIGHT */
                PEPPER_TRACE("right click\n");
                event.index = 3;
                break;
            default:
                PEPPER_TRACE("wheel or something pressed\n");
                break;
            }
            event.type   = PEPPER_INPUT_EVENT_POINTER_BUTTON;
            event.time   = bp->time;
            event.serial = bp->sequence;
            event.state  = PEPPER_INPUT_EVENT_STATE_PRESSED;
            event.value  = 0;
            event.x      = bp->event_x;
            event.y      = bp->event_y;;
        }
        break;
    case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t *br = (xcb_button_release_event_t *)xev;
            switch (br->detail)
            {
            case XCB_BUTTON_INDEX_1:/* FIXME: LEFT */
                PEPPER_TRACE("left released\n");
                event.index = 1;
                break;
            case XCB_BUTTON_INDEX_3:/* FIXME: RIGHT */
                PEPPER_TRACE("right released\n");
                event.index = 3;
                break;
            default:
                PEPPER_TRACE("wheel or something pressed\n");
                break;
            }
            event.type   = PEPPER_INPUT_EVENT_POINTER_BUTTON;
            event.time   = br->time;
            event.serial = br->sequence;
            event.state  = PEPPER_INPUT_EVENT_STATE_RELEASED;
            event.value  = 0;
            event.x      = br->event_x;
            event.y      = br->event_y;;
        }
        break;
    case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)xev;

            event.type   = PEPPER_INPUT_EVENT_POINTER_MOTION;
            event.time   = motion->time;
            event.serial = motion->sequence;
            event.index  = 0;
            event.state  = 0;
            event.value  = 0;
            event.x      = motion->event_x;
            event.y      = motion->event_y;
        }
        break;
    default :
        PEPPER_ERROR("unknown input event, [0x%x]\n", type);
    }

    /* TODO: send input event */
}

void
x11_window_input_property_change(xcb_connection_t *conn, xcb_window_t window)
{
     const static uint32_t values[] =
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
    seat->pointer = pepper_pointer_device_create(conn->compositor);
    if (!seat->pointer)
    {
        PEPPER_ERROR("failed to create pepper pointer device\n");
        /* TODO: error handling */
    }
    seat->caps |= WL_SEAT_CAPABILITY_POINTER;
    seat->keyboard = pepper_keyboard_device_create(conn->compositor);
    if (!seat->keyboard)
    {
        PEPPER_ERROR("failed to create pepper keyboard device\n");
        /* TODO: error handling */
    }
    seat->caps |= WL_SEAT_CAPABILITY_KEYBOARD;

    /* x-connection has only 1 seat */
    conn->seat = seat;
}
