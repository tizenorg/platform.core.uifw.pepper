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
#include <string.h>

void
pepper_view_mark_dirty(pepper_view_t *view, uint32_t flag)
{
	pepper_view_t  *child;
	int             i;

	if (view->dirty & flag)
		return;

	view->dirty |= flag;

	/* Mark entire subtree's geometry as dirty. */
	if (flag & PEPPER_VIEW_GEOMETRY_DIRTY) {
		pepper_list_for_each(child, &view->children_list, parent_link)
		pepper_view_mark_dirty(child, PEPPER_VIEW_GEOMETRY_DIRTY);

		for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
			view->plane_entries[i].need_transform_update = PEPPER_TRUE;
	}

	/* Mark entire subtree's active as dirty. */
	if (flag & PEPPER_VIEW_ACTIVE_DIRTY) {
		pepper_list_for_each(child, &view->children_list, parent_link)
		pepper_view_mark_dirty(child, PEPPER_VIEW_ACTIVE_DIRTY);
	}

	pepper_compositor_schedule_repaint(view->compositor);
}

void
pepper_view_surface_damage(pepper_view_t *view)
{
	int i;

	for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++) {
		pepper_plane_entry_t *entry = &view->plane_entries[i];

		if (entry->plane) {
			pixman_region32_t damage;

			pixman_region32_init(&damage);
			pixman_region32_copy(&damage, &view->surface->damage_region);
			pixman_region32_intersect_rect(&damage, &damage, 0, 0, view->w, view->h);

			pepper_transform_pixman_region(&damage, &view->global_transform);
			pixman_region32_translate(&damage,
						  -entry->plane->output->geometry.x,
						  -entry->plane->output->geometry.y);
			pixman_region32_intersect(&damage, &damage, &entry->base.visible_region);
			pepper_plane_add_damage_region(entry->plane, &damage);
		}
	}
}

static pepper_list_t *
view_insert(pepper_view_t *view, pepper_list_t *pos, pepper_bool_t subtree)
{
	if ((pos != &view->compositor_link) && (pos->next != &view->compositor_link)) {
		pepper_list_remove(&view->compositor_link);
		pepper_list_insert(pos, &view->compositor_link);
		pepper_object_emit_event(&view->base, PEPPER_EVENT_VIEW_STACK_CHANGE, NULL);
		pepper_view_mark_dirty(view, PEPPER_VIEW_Z_ORDER_DIRTY);
	}

	pos = &view->compositor_link;

	if (subtree) {
		pepper_view_t *child;

		pepper_list_for_each(child, &view->children_list, parent_link)
		pos = view_insert(child, pos, subtree);
	}

	return pos;
}

static void
plane_entry_set_plane(pepper_plane_entry_t *entry, pepper_plane_t *plane)
{
	if (entry->plane == plane)
		return;

	if (entry->plane) {
		pepper_plane_add_damage_region(entry->plane, &entry->base.visible_region);
		entry->plane = NULL;
		pixman_region32_fini(&entry->base.visible_region);
	}

	entry->plane = plane;

	if (entry->plane) {
		pixman_region32_init(&entry->base.visible_region);
		entry->need_damage = PEPPER_TRUE;
	}
}

/**
 * Assign a view to a plane.
 *
 * @param view      view to assign
 * @param output    output of the plane
 * @param plane     plane to assign a view
 */
PEPPER_API void
pepper_view_assign_plane(pepper_view_t *view, pepper_output_t *output,
			 pepper_plane_t *plane)
{
	PEPPER_CHECK(!plane ||
		     plane->output == output, return, "Plane output mismatch.\n");
	plane_entry_set_plane(&view->plane_entries[output->id], plane);
}

void
pepper_view_update(pepper_view_t *view)
{
	pepper_bool_t   active;
	int             i;
	uint32_t        output_overlap_prev;

	if (!view->dirty)
		return;

	/* Update parent view first as transform and active flag are affected by the parent. */
	if (view->parent) {
		pepper_view_update(view->parent);
		active = view->parent->active && view->mapped;
	} else {
		active = view->mapped;
	}

	if (view->active == active)
		view->dirty &= ~PEPPER_VIEW_ACTIVE_DIRTY;

	if (!view->dirty)
		return;

	view->active = active;

	/* Damage for the view unmap will be handled by assigning NULL plane. */
	if (!view->active)
		return;

	/* We treat the modification as unmapping and remapping the view. So,
	 * damage for the unmap and damage for the remap.
	 *
	 * Here, we know on which planes the view was previously located. So, we can
	 * inflict damage on the planes for the unmap.
	 *
	 * However, new visible region of the view is not known at the moment
	 * because no plane is assigned yet. So, simply mark the all plane entries
	 * as damaged and the damage for the remap will be inflicted separately for
	 * each output when the visible region is calculated on output repaint.
	 */

	for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++) {
		pepper_plane_entry_t *entry = &view->plane_entries[i];

		if (entry->plane)
			pepper_plane_add_damage_region(entry->plane, &entry->base.visible_region);
	}

	/* Update geometry. */
	if (view->dirty & PEPPER_VIEW_GEOMETRY_DIRTY) {
		pepper_output_t *output;

		/* Transform. */
		pepper_mat4_init_translate(&view->global_transform, view->x, view->y, 0.0);
		pepper_mat4_multiply(&view->global_transform, &view->global_transform,
				     &view->transform);

		if (view->inherit_transform && view->parent) {
			pepper_mat4_multiply(&view->global_transform,
					     &view->parent->global_transform, &view->global_transform);
		}

		pepper_mat4_inverse(&view->global_transform_inverse, &view->global_transform);

		/* Bounding region. */
		pixman_region32_fini(&view->bounding_region);
		pixman_region32_init_rect(&view->bounding_region, 0, 0, view->w, view->h);
		pepper_transform_pixman_region(&view->bounding_region, &view->global_transform);

		/* Opaque region. */
		if (view->surface && pepper_mat4_is_translation(&view->global_transform)) {
			pixman_region32_copy(&view->opaque_region, &view->surface->opaque_region);
			pixman_region32_translate(&view->opaque_region,
						  view->global_transform.m[12], view->global_transform.m[13]);
		} else {
			pixman_region32_clear(&view->opaque_region);
		}

		/* Output overlap. */
		output_overlap_prev = view->output_overlap;
		view->output_overlap = 0;

		pepper_list_for_each(output, &view->compositor->output_list, link) {
			pixman_box32_t   box = {
				output->geometry.x,
				output->geometry.y,
				output->geometry.x + output->geometry.w,
				output->geometry.y + output->geometry.h
			};

			if (pixman_region32_contains_rectangle(&view->bounding_region,
							       &box) != PIXMAN_REGION_OUT) {
				view->output_overlap |= (1 << output->id);
				if (!(output_overlap_prev & (1 << output->id)))
					pepper_surface_send_enter(view->surface, output);
			} else {
				if (view->surface && (output_overlap_prev & (1 << output->id)))
					pepper_surface_send_leave(view->surface, output);
			}
		}
	}

	/* Mark the plane entries as damaged. */
	for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
		view->plane_entries[i].need_damage = PEPPER_TRUE;

	view->active = active;
	view->dirty = 0;
}

static void
view_init(pepper_view_t *view, pepper_compositor_t *compositor)
{
	int i;

	view->compositor_link.item = view;
	view->parent_link.item = view;
	view->link.item = view;
	view->surface_link.item = view;

	view->compositor = compositor;
	pepper_list_insert(&compositor->view_list, &view->compositor_link);

	pepper_list_init(&view->children_list);

	pepper_mat4_init_identity(&view->transform);
	pepper_mat4_init_identity(&view->global_transform);
	pixman_region32_init(&view->bounding_region);
	pixman_region32_init(&view->opaque_region);

	for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++) {
		view->plane_entries[i].base.view = view;
		view->plane_entries[i].link.item = &view->plane_entries[i];
	}
}

/**
 * Create and add a view to the given compositor
 *
 * @param compositor    compositor object
 *
 * @return the created view
 */
PEPPER_API pepper_view_t *
pepper_compositor_add_view(pepper_compositor_t *compositor)
{
	pepper_view_t *view = (pepper_view_t *)pepper_object_alloc(PEPPER_OBJECT_VIEW,
			      sizeof(pepper_view_t));
	PEPPER_CHECK(view, return NULL, "pepper_object_alloc() failed.\n");

	view_init(view, compositor);

	view->x = 0.0;
	view->y = 0.0;
	view->w = 0;
	view->h = 0;

	pepper_object_emit_event(&compositor->base, PEPPER_EVENT_COMPOSITOR_VIEW_ADD,
				 view);
	return view;
}

static void
view_unlink_from_surface(pepper_view_t *view)
{
	pepper_list_remove(&view->surface_link);

	subsurface_destroy_children_views(view->surface->sub, view);
}

static void
view_link_to_surface(pepper_view_t *view)
{
	pepper_list_insert(&view->surface->view_list, &view->surface_link);

	subsurface_create_children_views(view->surface->sub, view);
}

/**
 * Set a surface to the given view as its content
 *
 * @param view      view object
 * @param surface   surface object
 *
 * @return PEPPER_TRUE on success, PEPPER_FALSE otherwise
 *
 * View is just a container which can be located on the compositor space. Its content come from
 * other resources like wl_surface. This function sets the content of the given view with the given
 * surface.
 */
PEPPER_API pepper_bool_t
pepper_view_set_surface(pepper_view_t *view, pepper_surface_t *surface)
{
	if (view->surface == surface)
		return PEPPER_TRUE;

	if (view->surface)
		view_unlink_from_surface(view);

	view->surface = surface;

	if (view->surface)
		view_link_to_surface(view);

	pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
	return PEPPER_TRUE;
}

/**
 * Destroy the given view
 *
 * @param view  view object
 */
PEPPER_API void
pepper_view_destroy(pepper_view_t *view)
{
	int             i;
	pepper_view_t  *child, *tmp;

	pepper_object_emit_event(&view->compositor->base,
				 PEPPER_EVENT_COMPOSITOR_VIEW_REMOVE, view);
	pepper_object_fini(&view->base);

	for (i = 0; i < PEPPER_MAX_OUTPUT_COUNT; i++)
		plane_entry_set_plane(&view->plane_entries[i], NULL);

	pepper_list_for_each_safe(child, tmp, &view->children_list, parent_link)
	pepper_view_destroy(child);

	if (view->parent)
		pepper_list_remove(&view->parent_link);

	pepper_list_remove(&view->compositor_link);

	if (view->surface)
		pepper_list_remove(&view->surface_link);

	pixman_region32_fini(&view->opaque_region);
	pixman_region32_fini(&view->bounding_region);

	free(view);
}

/**
 * Get the compositor of the given view
 *
 * @param view  view object
 *
 * @return compositor of the view
 */
PEPPER_API pepper_compositor_t *
pepper_view_get_compositor(pepper_view_t *view)
{
	return view->compositor;
}

/**
 * Get the surface of the given view
 *
 * @param view  view object
 *
 * @return surface of the view
 *
 * @see pepper_view_set_surface()
 */
PEPPER_API pepper_surface_t *
pepper_view_get_surface(pepper_view_t *view)
{
	return view->surface;
}

/**
 * Set the parent of the given view
 *
 * @param view      view object
 * @param parent    parent view object
 *
 * Views can inherit some of the properties from their parent. Changing the parent of a view might
 * result repaint.
 *
 * @see pepper_view_set_transform_inherit()
 * @see pepper_view_get_parent()
 * @see pepper_view_get_children_list()
 * @see pepper_view_stack_above()
 * @see pepper_view_stack_below()
 * @see pepper_view_stack_top()
 * @see pepper_view_stack_bottom()
 */
PEPPER_API void
pepper_view_set_parent(pepper_view_t *view, pepper_view_t *parent)
{
	if (view->parent == parent)
		return;

	if (view->parent)
		pepper_list_remove(&view->parent_link);

	view->parent = parent;

	if (view->parent)
		pepper_list_insert(view->parent->children_list.prev, &view->parent_link);

	pepper_view_mark_dirty(view,
			       PEPPER_VIEW_ACTIVE_DIRTY | PEPPER_VIEW_GEOMETRY_DIRTY);
}

/**
 * Get the parent of the given view
 *
 * @param view view object
 *
 * @return the parent view
 *
 * @see pepper_view_set_parent()
 */
PEPPER_API pepper_view_t *
pepper_view_get_parent(pepper_view_t *view)
{
	return view->parent;
}

/**
 * Set the transform inheritance flag of the given view
 *
 * @param view      view object
 * @param inherit   boolean value to enable inherit or not
 *
 * If the inherit is set to PEPPER_TRUE, view position and transform is interpreted as
 * relative to its parent. If it is not, position and transform is relative to the global frame.
 *
 * @see pepper_view_set_parent()
 * @see pepper_view_get_transform_inherit()
 */
PEPPER_API void
pepper_view_set_transform_inherit(pepper_view_t *view, pepper_bool_t inherit)
{
	if (view->inherit_transform == inherit)
		return;

	if (view->inherit_transform) {
		/* Inherit flag changed from TRUE to FALSE.
		 * We have to update view position and transform from parent local to global. */
		view->x = view->global_transform.m[12];
		view->y = view->global_transform.m[13];

		pepper_mat4_copy(&view->transform, &view->global_transform);
		pepper_mat4_translate(&view->transform, -view->x, -view->y, 0.0);
	} else {
		/* Inherit flag changed from FALSE to TRUE.
		 * We have to update view position and transform from global to parent local. */

		if (view->parent) {
			/* Get transform matrix on the parent local coordinate space. */
			pepper_mat4_inverse(&view->transform, &view->parent->global_transform);
			pepper_mat4_multiply(&view->transform, &view->global_transform,
					     &view->transform);

			/* Set position of the (x, y) translation term of the matrix. */
			view->x = view->transform.m[12];
			view->y = view->transform.m[13];

			/* Compensate the view position translation. */
			pepper_mat4_translate(&view->transform, -view->x, -view->y, 0.0);
		}
	}

	view->inherit_transform = inherit;
}

/**
 * Get the transform inheritance flag of the given view
 *
 * @param view  view object
 *
 * @return transform inheritance flag
 *
 * @see pepper_view_set_transform_inherit()
 */
PEPPER_API pepper_bool_t
pepper_view_get_transform_inherit(pepper_view_t *view)
{
	return view->inherit_transform;
}

/**
 * Stack the given view above the target view
 *
 * @param view      view object
 * @param below     target view to stack the given view above it
 * @param subtree   flag for stacking entire subtree or not
 *
 * @return PEPPER_TRUE on success, PEPPER_FALSE otherwise
 *
 * If the subtree is PEPPER_TRUE, entire subtree is taken from the tack, and inserted above the
 * target view. Child views are stacked above their parents. Z-order between siblings is determined
 * by the order in the list.
 *
 * @see pepper_view_stack_below()
 * @see pepper_view_stack_top()
 * @see pepper_view_stack_bottom()
 */
PEPPER_API pepper_bool_t
pepper_view_stack_above(pepper_view_t *view, pepper_view_t *below,
			pepper_bool_t subtree)
{
	view_insert(view, below->compositor_link.prev, subtree);
	return PEPPER_TRUE;
}

/**
 * Stack the given view below the target view
 *
 * @param view      view object
 * @param above     target view to stack the given view below it
 * @param subtree   flag for stacking entire subtree or not
 *
 * @return PEPPER_TRUE on success, PEPPER_FALSE otherwise
 *
 * @see pepper_view_stack_above()
 * @see pepper_view_stack_top()
 * @see pepper_view_stack_bottom()
 */
PEPPER_API pepper_bool_t
pepper_view_stack_below(pepper_view_t *view, pepper_view_t *above,
			pepper_bool_t subtree)
{
	view_insert(view, &above->compositor_link, subtree);
	return PEPPER_TRUE;
}

/**
 * Stack the given view at the top
 *
 * @param view      view object
 * @param subtree   flag for stacking entire subtree or not
 *
 * @see pepper_view_stack_above()
 * @see pepper_view_stack_below()
 * @see pepper_view_stack_bottom()
 */
PEPPER_API void
pepper_view_stack_top(pepper_view_t *view, pepper_bool_t subtree)
{
	view_insert(view, &view->compositor->view_list, subtree);
}

/**
 * Stack the given view at the bottom
 *
 * @param view      view object
 * @param subtree   flag for stacking entire subtree or not
 *
 * @see pepper_view_stack_above()
 * @see pepper_view_stack_below()
 * @see pepper_view_stack_top()
 */
PEPPER_API void
pepper_view_stack_bottom(pepper_view_t *view, pepper_bool_t subtree)
{
	view_insert(view, view->compositor->view_list.prev, subtree);
}

/**
 * Get the view right above the given view
 *
 * @param view  view object
 *
 * @return the view right above the given view
 *
 * @see pepper_view_get_below()
 */
PEPPER_API pepper_view_t *
pepper_view_get_above(pepper_view_t *view)
{
	return view->compositor_link.next->item;
}

/**
 * Get the view right below the given view
 *
 * @param view  view object
 *
 * @return the view right below the given view
 *
 * @see pepper_view_get_below()
 */
PEPPER_API pepper_view_t *
pepper_view_get_below(pepper_view_t *view)
{
	return view->compositor_link.prev->item;
}

/**
 * Get the list of children of the given view
 *
 * @param view  view object
 *
 * @return the children list
 *
 * @see pepper_view_set_parent()
 */
PEPPER_API const pepper_list_t *
pepper_view_get_children_list(pepper_view_t *view)
{
	return &view->children_list;
}

/**
 * Resize the given view (Don't use this function)
 *
 * @param view  view object
 * @param w     width of the new size
 * @param h     height of the new size
 *
 * Never use this function. The view size is automatically determined by the surface.
 */
PEPPER_API void
pepper_view_resize(pepper_view_t *view, int w, int h)
{
	if (view->w == w && view->h == h)
		return;

	view->w = w;
	view->h = h;
	pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
}

/**
 * Get the size of the given view
 *
 * @param view  view object
 * @param w     pointer to receive width
 * @param h     pointer to receive height
 */
PEPPER_API void
pepper_view_get_size(pepper_view_t *view, int *w, int *h)
{
	if (w)
		*w = view->w;

	if (h)
		*h = view->h;
}

/**
 * Set the position of the given view
 *
 * @param view  view object
 * @param x     x coordinate of the new position
 * @param y     y coordinate of the new position
 *
 * The position can be interpreted differently according to the transform inheritance flag of the
 * view.
 *
 * @see pepper_view_set_transform_inherit()
 * @see pepper_view_get_position()
 */
PEPPER_API void
pepper_view_set_position(pepper_view_t *view, double x, double y)
{
	if (view->x == x && view->y == y)
		return;

	view->x = x;
	view->y = y;
	pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
}

/**
 * Get the position of the given view
 *
 * @param view  view object
 * @param x     pointer to receive x coordinate
 * @param y     pointer to receive y coordinate
 *
 * @see pepper_view_set_position()
 */
PEPPER_API void
pepper_view_get_position(pepper_view_t *view, double *x, double *y)
{
	if (x)
		*x = view->x;

	if (y)
		*y = view->y;
}

/**
 * Set the transform matrix of the given view
 *
 * @param view      view object
 * @param matrix    4x4 transform matrix
 *
 * The transform might be relative to its parent or global frame.
 *
 * @see pepper_view_set_transform_inherit()
 */
PEPPER_API void
pepper_view_set_transform(pepper_view_t *view, const pepper_mat4_t *matrix)
{
	pepper_mat4_copy(&view->transform, matrix);
	pepper_view_mark_dirty(view, PEPPER_VIEW_GEOMETRY_DIRTY);
}

/**
 * Get the transform matrix of the given view
 *
 * @param view  view object
 *
 * @return the transform matrix
 */
PEPPER_API const pepper_mat4_t *
pepper_view_get_transform(pepper_view_t *view)
{
	return &view->transform;
}

/**
 * Map the given view
 *
 * @param view  view object
 *
 * View is visible if it is mapped and its parent is visible.
 *
 * @see pepper_view_unmap()
 */
PEPPER_API void
pepper_view_map(pepper_view_t *view)
{
	if (view->mapped)
		return;

	view->mapped = PEPPER_TRUE;
	pepper_view_mark_dirty(view, PEPPER_VIEW_ACTIVE_DIRTY);
}

/**
 * Unmap the given view
 *
 * @param view  view object
 *
 * @see pepper_view_map()
 */
PEPPER_API void
pepper_view_unmap(pepper_view_t *view)
{
	if (!view->mapped)
		return;

	view->mapped = PEPPER_FALSE;
	pepper_view_mark_dirty(view, PEPPER_VIEW_ACTIVE_DIRTY);
}

/**
 * Check if the view is mapped
 *
 * @param view  view object
 *
 * @return PEPPER_TRUE if the view is mapped, PEPPER_FALSE otherwise
 *
 * @see pepper_view_map()
 * @see pepper_view_unmap()
 * @see pepper_view_is_visible()
 */
PEPPER_API pepper_bool_t
pepper_view_is_mapped(pepper_view_t *view)
{
	return view->mapped;
}

/**
 * Check if the view is visible
 *
 * @param view  view object
 *
 * @return PEPPER_TRUE if the view is visible, PEPPER_FALSE otherwise
 *
 * Here, visible means that all its parent and direct ancestors are mapped. The visility is not
 * affected by the views obscuring the given view.
 *
 * @see pepper_view_map()
 * @see pepper_view_unmap()
 * @see pepper_view_is_mapped()
 */
PEPPER_API pepper_bool_t
pepper_view_is_visible(pepper_view_t *view)
{
	if (view->parent)
		return pepper_view_is_visible(view->parent) && view->mapped;

	return view->mapped;
}

/**
 * Check if the view is opaque
 *
 * @param view  view object
 *
 * @return PEPPER_TRUE if the view is opaque, PEPPER_FALSE otherwise
 *
 * If the content is opaque or opaque region of the surface covers entire region, the view is
 * opaque.
 */
PEPPER_API pepper_bool_t
pepper_view_is_opaque(pepper_view_t *view)
{
	pepper_surface_t       *surface = view->surface;
	struct wl_shm_buffer   *shm_buffer = wl_shm_buffer_get(
			surface->buffer.buffer->resource);
	pixman_box32_t          extent;

	if (shm_buffer) {
		uint32_t shm_format = wl_shm_buffer_get_format(shm_buffer);

		if (shm_format == WL_SHM_FORMAT_XRGB8888 || shm_format == WL_SHM_FORMAT_RGB565)
			return PEPPER_TRUE;
	}

	/* TODO: format check for wl_drm or wl_tbm?? */

	extent.x1 = 0;
	extent.y1 = 0;
	extent.x2 = view->surface->w;
	extent.y2 = view->surface->h;

	if (pixman_region32_contains_rectangle(&surface->opaque_region,
					       &extent) == PIXMAN_REGION_IN)
		return PEPPER_TRUE;

	return PEPPER_FALSE;
}

/**
 * Get the view local coordinates for the given global coordinates
 *
 * @param view  view object
 * @param gx    x coordinate in global space
 * @param gy    y coordinate in global space
 * @param lx    pointer to receive x coordinate in view local space
 * @param ly    pointer to receive y coordinate in view local space
 */
PEPPER_API void
pepper_view_get_local_coordinate(pepper_view_t *view, double gx, double gy,
				 double *lx, double *ly)
{
	pepper_vec4_t pos = { gx, gy, 0.0, 1.0 };

	pepper_mat4_transform_vec4(&view->global_transform_inverse, &pos);

	PEPPER_ASSERT(pos.w >= 1e-6);

	*lx = pos.x / pos.w;
	*ly = pos.y / pos.w;
}

/**
 * Get the global coodinates for the given local coordinates
 *
 * @param view  view object
 * @param lx    x coordinate in view local space
 * @param ly    y coordinate in view local space
 * @param gx    pointer to receive x coordinate in global space
 * @param gy    pointer to receive y coordinate in global space
 */
PEPPER_API void
pepper_view_get_global_coordinate(pepper_view_t *view, double lx, double ly,
				  double *gx, double *gy)
{
	pepper_vec4_t pos = { lx, ly, 0.0, 1.0 };

	pepper_mat4_transform_vec4(&view->global_transform, &pos);

	PEPPER_ASSERT(pos.w >= 1e-6);

	*gx = pos.x / pos.w;
	*gy = pos.y / pos.w;
}
