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

#ifndef PEPPER_INTERNAL_H
#define PEPPER_INTERNAL_H

#include <sys/un.h>
#include <config.h>
#include "pepper.h"
#include <wayland-util.h>
#include <pixman.h>
#include "pepper-output-backend.h"
#include "pepper-input-backend.h"

#define PEPPER_OBJECT_BUCKET_BITS       5
#define PEPPER_MAX_OUTPUT_COUNT         32
#define PEPPER_OUTPUT_MAX_TICK_COUNT    10

typedef struct pepper_region        pepper_region_t;
typedef struct pepper_surface_state pepper_surface_state_t;
typedef struct pepper_plane_entry   pepper_plane_entry_t;
typedef struct pepper_input         pepper_input_t;
typedef struct pepper_touch_point   pepper_touch_point_t;

struct pepper_object {
	pepper_object_type_t    type;
	uint32_t                id;
	pepper_map_t            user_data_map;
	pepper_map_entry_t     *buckets[1 << PEPPER_OBJECT_BUCKET_BITS];
	pepper_list_t           event_listener_list;
};

pepper_object_t *
pepper_object_alloc(pepper_object_type_t type, size_t size);

void
pepper_object_init(pepper_object_t *object, pepper_object_type_t type);

void
pepper_object_fini(pepper_object_t *object);

struct pepper_event_listener {
	pepper_object_t             *object;
	uint32_t                    id;
	int                         priority;
	pepper_event_callback_t     callback;
	void                       *data;

	pepper_list_t               link;
};

/* compositor */
struct pepper_compositor {
	pepper_object_t          base;
	char                    *socket_name;
	struct wl_display       *display;
	struct wl_global        *global;
	struct wl_list           resource_list;

	pepper_list_t            surface_list;
	pepper_list_t            region_list;
	pepper_list_t            seat_list;
	pepper_list_t            output_list;
	pepper_list_t            view_list;
	pepper_list_t            input_device_list;
	pepper_subcompositor_t  *subcomp;

	uint32_t                 output_id_allocator;
	pepper_bool_t            update_scheduled;

	clockid_t                clock_id;
	pepper_bool_t            clock_used;

	struct sockaddr_un       addr;
};

void
pepper_compositor_schedule_repaint(pepper_compositor_t *compositor);

struct pepper_output {
	pepper_object_t             base;
	pepper_compositor_t        *compositor;
	uint32_t                    id;
	char                       *name;

	struct wl_global           *global;
	struct wl_list              resource_list;
	pepper_list_t               link;

	pepper_output_geometry_t    geometry;
	int32_t                     scale;

	pepper_output_mode_t        current_mode;

	/* Backend-specific variables. */
	pepper_output_backend_t    *backend;
	void                       *data;

	/* Frame state flags. */
	struct {
		pepper_bool_t           scheduled;
		pepper_bool_t           pending;
		struct timespec         time;
		uint32_t                count;

		/* For FPS measuring */
		pepper_bool_t           print_fps;
		uint32_t                ticks[PEPPER_OUTPUT_MAX_TICK_COUNT];
		int                     tick_index;
		uint32_t                total_time;
	} frame;

	pepper_list_t               plane_list;
	pepper_list_t               view_list;
};

void
pepper_output_schedule_repaint(pepper_output_t *output);

struct pepper_buffer {
	pepper_object_t         base;
	struct wl_resource     *resource;

	int                     ref_count;
	struct wl_listener      resource_destroy_listener;

	/* the buffer size is unknown until it is actually attached to a renderer. */
	int32_t                 w, h;
	pepper_bool_t           attached;
};

pepper_buffer_t *
pepper_buffer_from_resource(struct wl_resource *resource);

struct pepper_surface_state {
	pepper_buffer_t            *buffer;
	int32_t                     x;
	int32_t                     y;
	pepper_bool_t               newly_attached;

	int32_t                     transform;
	int32_t                     scale;

	pixman_region32_t           damage_region;
	pixman_region32_t           opaque_region;
	pixman_region32_t           input_region;

	struct wl_list              frame_callback_list;
	pepper_event_listener_t    *buffer_destroy_listener;
};

void
pepper_surface_state_init(pepper_surface_state_t *state);

void
pepper_surface_state_fini(pepper_surface_state_t *state);

struct pepper_surface {
	pepper_object_t         base;
	pepper_compositor_t    *compositor;
	struct wl_resource     *resource;
	pepper_list_t           link;

	struct {
		pepper_buffer_t         *buffer;
		pepper_bool_t            has_ref;
		pepper_event_listener_t *destroy_listener;
		int32_t                  x, y;
		int32_t                  transform;
		int32_t                  scale;
	} buffer;

	/* Surface size in surface local coordinate space.
	 * Buffer is transformed and scaled into surface local coordinate space. */
	int32_t                 w, h;

	pixman_region32_t       damage_region;
	pixman_region32_t       opaque_region;
	pixman_region32_t       input_region;
	pepper_bool_t           pickable;

	struct wl_list          frame_callback_list;

	/* Surface states. wl_surface.commit will apply the pending state into current. */
	pepper_surface_state_t  pending;

	char                   *role;
	pepper_list_t           view_list;

	pepper_list_t           subsurface_list;
	pepper_list_t           subsurface_pending_list;
	pepper_subsurface_t    *sub;
};

pepper_surface_t *
pepper_surface_create(pepper_compositor_t *compositor,
		      struct wl_client *client,
		      struct wl_resource *resource,
		      uint32_t id);

void
pepper_surface_destroy(pepper_surface_t *surface);

void
pepper_surface_commit(pepper_surface_t *surface);

void
pepper_surface_commit_state(pepper_surface_t *surface,
			    pepper_surface_state_t *state);

void
pepper_surface_send_frame_callback_done(pepper_surface_t *surface,
					uint32_t time);

struct pepper_region {
	pepper_object_t         base;
	pepper_compositor_t    *compositor;
	struct wl_resource     *resource;
	pepper_list_t           link;

	pixman_region32_t       pixman_region;
};

pepper_region_t *
pepper_region_create(pepper_compositor_t *compositor,
		     struct wl_client *client,
		     struct wl_resource *resource,
		     uint32_t id);

void
pepper_region_destroy(pepper_region_t *region);

void
pepper_transform_pixman_region(pixman_region32_t *region,
			       const pepper_mat4_t *matrix);

/* Subcompositor */
struct pepper_subcompositor {
	pepper_object_t          base;
	pepper_compositor_t     *compositor;
	struct wl_global        *global;
	struct wl_list           resource_list;
};

pepper_subcompositor_t *
pepper_subcompositor_create(pepper_compositor_t *compositor);

void
pepper_subcompositor_destroy(pepper_subcompositor_t *subcompositor);

/* Subsurface */
struct pepper_subsurface {
	pepper_surface_t        *surface;
	pepper_surface_t        *parent;
	struct wl_resource      *resource;

	double                   x, y;
	pepper_list_t            children_list;

	pepper_list_t            parent_link;       /* link to parent's children_list */
	pepper_list_t            self_link;         /* link to its own children_list */

	/* This state is applied when the parent surface's wl_surface state is applied,
	 * regardless of the sub-surface's mode. */
	struct {
		double               x, y;
		pepper_list_t        children_list;

		pepper_list_t        parent_link;
		pepper_list_t        self_link;
	} pending;

	pepper_bool_t            need_restack;

	pepper_bool_t            sync;          /* requested commit behavior */

	/* In sync mode, wl_surface.commit will apply the pending state into cache.
	 * And cached state will flush into surface's current when parent's wl_surface.commit called */
	pepper_surface_state_t   cache;
	pepper_bool_t            cached;

	pepper_event_listener_t *parent_destroy_listener;
	pepper_event_listener_t *parent_commit_listener;
};

pepper_subsurface_t *
pepper_subsurface_create(pepper_surface_t *surface, pepper_surface_t *parent,
			 struct wl_client *client, struct wl_resource *resource, uint32_t id);

pepper_bool_t
pepper_subsurface_commit(pepper_subsurface_t *subsurface);

void
pepper_subsurface_destroy(pepper_subsurface_t *subsurface);

void
subsurface_destroy_children_views(pepper_subsurface_t *subsurface,
				  pepper_view_t *parent_view);

void
subsurface_create_children_views(pepper_subsurface_t *subsurface,
				 pepper_view_t *parent_view);

/* Input */
struct pepper_pointer {
	pepper_object_t                 base;
	pepper_seat_t                  *seat;
	struct wl_list                  resource_list;

	pepper_view_t                  *focus;
	pepper_event_listener_t        *focus_destroy_listener;
	uint32_t                        focus_serial;

	const pepper_pointer_grab_t    *grab;
	void                           *data;

	uint32_t                        time;
	double                          x, y;
	double                          vx, vy;

	struct {
		double                      x0, y0;
		double                      x1, y1;
	} clamp;

	double                          x_velocity;
	double                          y_velocity;

	pepper_view_t                  *cursor_view;
	int32_t                         hotspot_x;
	int32_t                         hotspot_y;
};

pepper_pointer_t *
pepper_pointer_create(pepper_seat_t *seat);

void
pepper_pointer_destroy(pepper_pointer_t *pointer);

void
pepper_pointer_bind_resource(struct wl_client *client,
			     struct wl_resource *resource, uint32_t id);

void
pepper_pointer_handle_event(pepper_pointer_t *pointer, uint32_t id,
			    pepper_input_event_t *event);

struct pepper_keyboard {
	pepper_object_t                 base;
	pepper_seat_t                  *seat;
	struct wl_list                  resource_list;

	pepper_view_t                  *focus;
	pepper_event_listener_t        *focus_destroy_listener;
	uint32_t                        focus_serial;

	const pepper_keyboard_grab_t   *grab;
	void                           *data;

	struct wl_array                 keys;

	struct xkb_keymap              *keymap;
	int                             keymap_fd;
	int                             keymap_len;
	struct xkb_keymap              *pending_keymap;

	struct xkb_state               *state;
	uint32_t                        mods_depressed;
	uint32_t                        mods_latched;
	uint32_t                        mods_locked;
	uint32_t                        group;
};

pepper_keyboard_t *
pepper_keyboard_create(pepper_seat_t *seat);

void
pepper_keyboard_destroy(pepper_keyboard_t *keyboard);

void
pepper_keyboard_bind_resource(struct wl_client *client,
			      struct wl_resource *resource, uint32_t id);

void
pepper_keyboard_handle_event(pepper_keyboard_t *keyboard, uint32_t id,
			     pepper_input_event_t *event);

struct pepper_touch_point {
	pepper_touch_t             *touch;

	uint32_t                    id;
	double                      x, y;

	pepper_view_t              *focus;
	uint32_t                    focus_serial;
	pepper_event_listener_t    *focus_destroy_listener;

	pepper_list_t               link;
};

struct pepper_touch {
	pepper_object_t             base;
	pepper_seat_t              *seat;
	struct wl_list              resource_list;

	pepper_list_t               point_list;

	const pepper_touch_grab_t  *grab;
	void                       *data;
};

pepper_touch_t *
pepper_touch_create(pepper_seat_t *seat);

void
pepper_touch_destroy(pepper_touch_t *touch);

void
pepper_touch_bind_resource(struct wl_client *client,
			   struct wl_resource *resource, uint32_t id);

void
pepper_touch_handle_event(pepper_touch_t *touch, uint32_t id,
			  pepper_input_event_t *event);

struct pepper_seat {
	pepper_object_t             base;
	pepper_compositor_t        *compositor;
	pepper_list_t               link;
	char                       *name;
	struct wl_global           *global;
	struct wl_list              resource_list;

	enum wl_seat_capability     caps;

	pepper_pointer_t           *pointer;
	pepper_keyboard_t          *keyboard;
	pepper_touch_t             *touch;

	pepper_list_t               input_device_list;
};

struct pepper_input_device {
	pepper_object_t                         base;
	pepper_compositor_t                    *compositor;
	pepper_list_t                           link;

	uint32_t                                caps;

	void                                   *data;
	const pepper_input_device_backend_t    *backend;
};

struct pepper_plane_entry {
	pepper_render_item_t        base;

	pepper_plane_t             *plane;
	pepper_bool_t               need_damage;
	pepper_bool_t               need_transform_update;

	pepper_list_t               link;
};

enum {
	PEPPER_VIEW_GEOMETRY_DIRTY      = 0x00000001,
	PEPPER_VIEW_ACTIVE_DIRTY        = 0x00000002,
	PEPPER_VIEW_Z_ORDER_DIRTY       = 0x00000004,
	PEPPER_VIEW_CONTENT_DIRTY       = 0x00000008,
};

struct pepper_view {
	pepper_object_t             base;
	pepper_compositor_t        *compositor;
	pepper_list_t               compositor_link;

	uint32_t                    dirty;

	/* Hierarchy. */
	pepper_view_t              *parent;
	pepper_list_t               parent_link;
	pepper_list_t               children_list;

	/* Geometry. */
	double                      x, y;
	int                         w, h;
	pepper_mat4_t               transform;
	pepper_bool_t               inherit_transform;

	pepper_mat4_t               global_transform;
	pepper_mat4_t               global_transform_inverse;

	pixman_region32_t           bounding_region;
	pixman_region32_t           opaque_region;

	/* Visibility. */
	pepper_bool_t               active;
	pepper_bool_t               prev_visible;
	pepper_bool_t               mapped;

	/* Content. */
	pepper_surface_t           *surface;
	pepper_list_t               surface_link;


	/* Output info. */
	uint32_t                    output_overlap;
	pepper_plane_entry_t        plane_entries[PEPPER_MAX_OUTPUT_COUNT];

	/* Temporary resource. */
	pepper_list_t               link;
};

void
pepper_view_mark_dirty(pepper_view_t *view, uint32_t flag);

void
pepper_view_update(pepper_view_t *view);

void
pepper_view_surface_damage(pepper_view_t *view);

struct pepper_plane {
	pepper_object_t     base;
	pepper_output_t    *output;

	pepper_list_t       entry_list;
	pixman_region32_t   damage_region;
	pixman_region32_t   clip_region;

	pepper_list_t       link;
};

pepper_object_t *
pepper_plane_create(pepper_object_t *output, pepper_object_t *above_plane);

void
pepper_plane_add_damage_region(pepper_plane_t *plane,
			       pixman_region32_t *region);

void
pepper_plane_update(pepper_plane_t *plane, const pepper_list_t *view_list,
		    pixman_region32_t *clip);

void
pepper_surface_flush_damage(pepper_surface_t *surface);

/* Misc. */
void
pepper_pixman_region_global_to_output(pixman_region32_t *region,
				      pepper_output_t *output);

void
pepper_transform_global_to_output(pepper_mat4_t *transform,
				  pepper_output_t *output);

#endif /* PEPPER_INTERNAL_H */
