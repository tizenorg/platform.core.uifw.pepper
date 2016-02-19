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
output_send_modes(pepper_output_t *output, struct wl_resource *resource)
{
	int i;
	int mode_count = output->backend->get_mode_count(output->data);

	for (i = 0; i < mode_count; i++) {
		pepper_output_mode_t mode;

		output->backend->get_mode(output->data, i, &mode);
		wl_output_send_mode(resource, mode.flags, mode.w, mode.h, mode.refresh);
	}

	wl_output_send_done(resource);
}

static void
output_update_mode(pepper_output_t *output)
{
	struct wl_resource *resource;
	int                 i;
	int                 mode_count = output->backend->get_mode_count(output->data);

	for (i = 0; i < mode_count; i++) {
		pepper_output_mode_t mode;

		output->backend->get_mode(output->data, i, &mode);

		if (mode.flags & WL_OUTPUT_MODE_CURRENT) {
			output->current_mode = mode;
			output->geometry.w = mode.w;
			output->geometry.h = mode.h;

			wl_resource_for_each(resource, &output->resource_list) {
				wl_output_send_mode(resource, mode.flags, mode.w, mode.h, mode.refresh);
				wl_output_send_done(resource);
			}
		}
	}
}

static void
output_send_geometry(pepper_output_t *output)
{
	struct wl_resource *resource;

	wl_resource_for_each(resource, &output->resource_list) {
		wl_output_send_geometry(resource,
					output->geometry.x, output->geometry.y,
					output->geometry.w, output->geometry.h,
					output->geometry.subpixel,
					output->geometry.maker, output->geometry.model,
					output->geometry.transform);
	}
}

static void
output_destroy(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
output_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct wl_resource      *resource;
	pepper_output_t         *output = (pepper_output_t *)data;

	resource = wl_resource_create(client, &wl_output_interface, 2, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&output->resource_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, NULL, NULL, output_destroy);

	wl_output_send_geometry(resource,
				output->geometry.x, output->geometry.y,
				output->geometry.w, output->geometry.h,
				output->geometry.subpixel,
				output->geometry.maker, output->geometry.model,
				output->geometry.transform);

	wl_output_send_scale(resource, output->scale);
	output_send_modes(output, resource);
}

static void
output_update_planes(pepper_output_t *output)
{
	pepper_plane_t     *plane;
	pixman_region32_t   clip;

	pixman_region32_init(&clip);

	pepper_list_for_each_reverse(plane, &output->plane_list, link)
	pepper_plane_update(plane, &output->view_list, &clip);

	pixman_region32_fini(&clip);
}

static void
output_repaint(pepper_output_t *output)
{
	pepper_view_t  *view;

	pepper_list_for_each(view, &output->compositor->view_list, compositor_link)
	pepper_view_update(view);

	pepper_list_init(&output->view_list);

	/* Build a list of views in sorted z-order that are visible on the given output. */
	pepper_list_for_each(view, &output->compositor->view_list, compositor_link) {
		if (!view->active || !(view->output_overlap & (1 << output->id)) ||
		    !view->surface) {
			/* Detach from the previously assigned plane. */
			pepper_view_assign_plane(view, output, NULL);
			continue;
		}

		pepper_list_insert(output->view_list.prev, &view->link);
	}

	output->backend->assign_planes(output->data, &output->view_list);
	output_update_planes(output);
	output->backend->repaint(output->data, &output->plane_list);

	output->frame.pending = PEPPER_TRUE;
	output->frame.scheduled = PEPPER_FALSE;

	pepper_list_for_each(view, &output->view_list, link) {
		/* TODO: Output time stamp and presentation feedback. */
		pepper_surface_send_frame_callback_done(view->surface,
							output->frame.time.tv_sec * 1000 +
							output->frame.time.tv_nsec / 1000000);
	}
}

static void
idle_repaint(void *data)
{
	pepper_output_t *output = data;
	output->backend->start_repaint_loop(output->data);
}

void
pepper_output_schedule_repaint(pepper_output_t *output)
{
	struct wl_event_loop   *loop;

	if (output->frame.scheduled)
		return;

	output->frame.scheduled = PEPPER_TRUE;

	if (output->frame.pending)
		return;

	/* Schedule on the next idle loop so that it can accumulate surface commits. */
	loop = wl_display_get_event_loop(output->compositor->display);
	wl_event_loop_add_idle(loop, idle_repaint, output);
}

/**
 * Add damage region to all planes in the given output.
 *
 * @param output    output object
 * @param region    damage region
 *
 * If the damage region is NULL, entire output area is marked as damaged.
 */
PEPPER_API void
pepper_output_add_damage_region(pepper_output_t *output,
				pixman_region32_t *region)
{
	pepper_plane_t *plane;
	pepper_list_for_each(plane, &output->plane_list, link)
	pepper_plane_add_damage_region(plane, region);
}

/**
 * Finish the currently pending frame of the given output.
 *
 * @param output    output object
 * @param ts        current time
 *
 * Output backend should call this function when they are ready to draw a new frame in response to
 * the requests from pepper library.
 */
PEPPER_API void
pepper_output_finish_frame(pepper_output_t *output, struct timespec *ts)
{
	struct timespec time;

	output->frame.pending = PEPPER_FALSE;

	if (ts)
		time = *ts;
	else
		pepper_compositor_get_time(output->compositor, &time);

	if (output->frame.print_fps) {
		if (output->frame.count > 0) {
			uint32_t tick = (time.tv_sec - output->frame.time.tv_sec) * 1000 +
					(time.tv_nsec - output->frame.time.tv_nsec) / 1000000;
			uint32_t tick_count;

			output->frame.total_time += tick;
			output->frame.total_time -= output->frame.ticks[output->frame.tick_index];
			output->frame.ticks[output->frame.tick_index] = tick;

			if (++output->frame.tick_index == PEPPER_OUTPUT_MAX_TICK_COUNT)
				output->frame.tick_index = 0;

			if (output->frame.count < PEPPER_OUTPUT_MAX_TICK_COUNT)
				tick_count = output->frame.count;
			else
				tick_count = PEPPER_OUTPUT_MAX_TICK_COUNT;

			PEPPER_TRACE("%s FPS : %.2f\n", output->name,
				     (double)(tick_count * 1000) / (double)output->frame.total_time);
		}
	}

	output->frame.count++;
	output->frame.time = time;

	if (output->frame.scheduled)
		output_repaint(output);
}

/**
 * Update mode of the given output.
 *
 * @param output    output object
 *
 * Backend should call this function after changing output mode.
 */
PEPPER_API void
pepper_output_update_mode(pepper_output_t *output)
{
	output_update_mode(output);
	pepper_object_emit_event(&output->base, PEPPER_EVENT_OUTPUT_MODE_CHANGE, NULL);
}

/**
 * Create an output and add it to the given compositor
 *
 * @param compositor    compositor object
 * @param backend       output backend function table
 * @param name          output name
 * @param data          backend private data
 * @param transform     output transform (ex. WL_OUTPUT_TRANSFORM_NORMAL)
 * @param scale         output scale
 *
 * @returns the created output object
 */
PEPPER_API pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t *compositor,
			     const pepper_output_backend_t *backend, const char *name, void *data,
			     int transform, int scale)
{
	pepper_output_t    *output;
	uint32_t            id;
	const char         *str;

	PEPPER_CHECK(name, return NULL, "Output name must be given.\n");

	pepper_list_for_each(output, &compositor->output_list, link) {
		PEPPER_CHECK(strcmp(output->name, name) != 0, return NULL,
			     "Output with name = %s already exist.\n", name);
	}

	id = ffs(~compositor->output_id_allocator);
	PEPPER_CHECK(id != 0, return NULL, "No available output ids.\n");

	id = id - 1;

	output = (pepper_output_t *)pepper_object_alloc(PEPPER_OBJECT_OUTPUT,
			sizeof(pepper_output_t));
	PEPPER_CHECK(output, return NULL, "pepper_object_alloc() failed.\n");

	output->compositor = compositor;
	output->link.item = output;
	wl_list_init(&output->resource_list);

	/* Create global object for this output. */
	output->global = wl_global_create(compositor->display, &wl_output_interface, 2,
					  output,
					  output_bind);
	if (!output->global) {
		free(output);
		return NULL;
	}

	output->id = id;
	compositor->output_id_allocator |= (1 << output->id);
	output->name = strdup(name);

	/* Create backend-side object. */
	output->backend = (pepper_output_backend_t *)backend;
	output->data = data;

	/* Initialize output modes. */
	output_update_mode(output);

	/* Initialize geometry. */
	output->geometry.transform = transform;
	output->scale = scale;
	output->geometry.subpixel = backend->get_subpixel_order(data);
	output->geometry.maker = backend->get_maker_name(data);
	output->geometry.model = backend->get_model_name(data);
	output->geometry.x = 0;
	output->geometry.y = 0;

	switch (transform) {
	case WL_OUTPUT_TRANSFORM_90:
	case WL_OUTPUT_TRANSFORM_FLIPPED_90:
	case WL_OUTPUT_TRANSFORM_270:
	case WL_OUTPUT_TRANSFORM_FLIPPED_270:
		output->geometry.w = output->current_mode.h / scale;
		output->geometry.h = output->current_mode.w / scale;
		break;
	default:
		output->geometry.w = output->current_mode.w / scale;
		output->geometry.h = output->current_mode.h / scale;
		break;
	}

	pepper_list_insert(&compositor->output_list, &output->link);
	pepper_list_init(&output->plane_list);

	/* FPS */
	str = getenv("PEPPER_DEBUG_FPS");
	if (str && atoi(str) != 0)
		output->frame.print_fps = PEPPER_TRUE;

	pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_OUTPUT_ADD,
				 output);
	return output;
}

/**
 * Get the compositor of the given output
 *
 * @param output    output object
 *
 * @return compositor of the output
 */
PEPPER_API pepper_compositor_t *
pepper_output_get_compositor(pepper_output_t *output)
{
	return output->compositor;
}

/**
 * Destroy the given output
 *
 * @param output    output object
 *
 * Destroying an output will emit PEPPER_EVENT_COMPOSITOR_OUTPUT_REMOVE event to the compositor.
 */
PEPPER_API void
pepper_output_destroy(pepper_output_t *output)
{
	pepper_object_emit_event(&output->compositor->base,
				 PEPPER_EVENT_COMPOSITOR_OUTPUT_REMOVE, output);
	pepper_object_fini(&output->base);

	output->compositor->output_id_allocator &= ~(1 << output->id);
	pepper_list_remove(&output->link);
	output->backend->destroy(output->data);
	wl_global_destroy(output->global);

	free(output->name);
	free(output);
}

/**
 * Get the list of wl_resource of the given output
 *
 * @param output    output object
 *
 * @return list of output resources
 */
PEPPER_API struct wl_list *
pepper_output_get_resource_list(pepper_output_t *output)
{
	return &output->resource_list;
}

/**
 * Move origin of the given output to the given position in global space
 *
 * @param output    output object
 * @param x         x coordinate of the new origin in global space
 * @param y         y coordinate of the new origin in global space
 *
 * Repaint is scheduled for the output if the origin of the output changes.
 */
PEPPER_API void
pepper_output_move(pepper_output_t *output, int32_t x, int32_t y)
{
	if ((output->geometry.x != x) || (output->geometry.y != y)) {
		output->geometry.x = x;
		output->geometry.y = y;

		/* TODO: pepper_output_add_damage_whole(out); */

		output_send_geometry(output);
		pepper_object_emit_event(&output->base, PEPPER_EVENT_OUTPUT_MOVE, NULL);
	}
}

/**
 * Get the geometry of the given output
 *
 * @param output    output object
 *
 * @return pointer to the structure of the output geometry
 */
PEPPER_API const pepper_output_geometry_t *
pepper_output_get_geometry(pepper_output_t *output)
{
	return &output->geometry;
}

/**
 * Get the scale value of the given output
 *
 * @param output    output object
 *
 * @return output scale
 */
PEPPER_API int32_t
pepper_output_get_scale(pepper_output_t *output)
{
	return output->scale;
}

/**
 * Get the number of available modes of the given output
 *
 * @param output    output object
 *
 * @return the number of available modes
 */
PEPPER_API int
pepper_output_get_mode_count(pepper_output_t *output)
{
	return output->backend->get_mode_count(output->data);
}

/**
 * Get the mode for the given mode index of the given output
 *
 * @param output    output object
 * @param index     index of the mode
 * @param mode      pointer to receive the mode info
 */
PEPPER_API void
pepper_output_get_mode(pepper_output_t *output, int index,
		       pepper_output_mode_t *mode)
{
	return output->backend->get_mode(output->data, index, mode);
}

/**
 * Get the current mode of the given output
 *
 * @param output    output object
 *
 * @return pointer to the current mode info
 */
PEPPER_API const pepper_output_mode_t *
pepper_output_get_current_mode(pepper_output_t *output)
{
	return &output->current_mode;
}

/**
 * Set display mode of the given output
 *
 * @param output    output object
 * @param mode      mode info
 *
 * @return PEPPER_TRUE on sucess, PEPPER_FALSE otherwise
 *
 * Mode setting might succeed even if the mode count is zero. There might be infinite available
 * modes for some output backends like x11 and wayland backend.
 */
PEPPER_API pepper_bool_t
pepper_output_set_mode(pepper_output_t *output,
		       const pepper_output_mode_t *mode)
{
	if (output->current_mode.w == mode->w && output->current_mode.h == mode->h &&
	    output->current_mode.refresh == mode->refresh)
		return PEPPER_TRUE;

	if (output->backend->set_mode(output->data, mode)) {
		/* TODO: pepper_output_add_damage_whole(out); */
		return PEPPER_TRUE;
	}

	return PEPPER_FALSE;
}

/**
 * Get the name of the given output
 *
 * @param output    output to get the name
 *
 * @return null terminating string of the output name
 */
PEPPER_API const char *
pepper_output_get_name(pepper_output_t *output)
{
	return output->name;
}

/**
 * Find the output for the given name in the given compositor
 *
 * @param compositor    compositor to find the output
 * @param name          name of the output to be found
 *
 * @return output with the given name if exist, NULL otherwise
 */
PEPPER_API pepper_output_t *
pepper_compositor_find_output(pepper_compositor_t *compositor, const char *name)
{
	pepper_output_t *output;

	pepper_list_for_each(output, &compositor->output_list, link) {
		if (strcmp(output->name, name) == 0)
			return output;
	}

	return NULL;
}
