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

void
pepper_plane_update(pepper_plane_t *plane, const pepper_list_t *view_list,
		    pixman_region32_t *clip)
{
	int                 x = plane->output->geometry.x;
	int                 y = plane->output->geometry.y;
	int                 w = plane->output->geometry.w;
	int                 h = plane->output->geometry.h;
	pixman_region32_t   plane_clip;
	pepper_view_t      *view;

	pixman_region32_init(&plane_clip);
	pepper_list_init(&plane->entry_list);

	pepper_list_for_each(view, view_list, link) {
		pepper_plane_entry_t *entry = &view->plane_entries[plane->output->id];

		if (entry->plane == plane) {
			pepper_list_insert(plane->entry_list.prev, &entry->link);

			if (entry->need_transform_update) {
				entry->base.transform = view->global_transform;
				pepper_transform_global_to_output(&entry->base.transform, plane->output);
				entry->need_transform_update = PEPPER_FALSE;
				pepper_mat4_inverse(&entry->base.inverse, &entry->base.transform);
			}

			/* Calculate visible region (output space). */
			pixman_region32_subtract(&entry->base.visible_region,
						 &view->bounding_region, &plane_clip);
			pixman_region32_intersect_rect(&entry->base.visible_region,
						       &entry->base.visible_region, x, y, w, h);
			pepper_pixman_region_global_to_output(&entry->base.visible_region,
							      plane->output);

			/* Accumulate opaque region of this view (global space). */
			pixman_region32_union(&plane_clip, &plane_clip, &view->opaque_region);

			/* Add damage for the new visible region. */
			if (entry->need_damage) {
				pepper_plane_add_damage_region(plane, &entry->base.visible_region);
				entry->need_damage = PEPPER_FALSE;
			}

			/* Flush surface damage. (eg. texture upload) */
			if (view->surface)
				pepper_surface_flush_damage(view->surface);
		}
	}

	/* Copy clip region of this plane. */
	pixman_region32_copy(&plane->clip_region, clip);

	/* Accumulate clip region obsecured by this plane. */
	pepper_pixman_region_global_to_output(&plane_clip, plane->output);
	pixman_region32_union(clip, clip, &plane_clip);
	pixman_region32_fini(&plane_clip);
}

/**
 * Create and add #pepper_plane_t to the output.
 *
 * @param output        output to add the plane
 * @param above_plane   added plane will be placed above above_plane
 *
 * @returns             #pepper_plane_t
 */
PEPPER_API pepper_plane_t *
pepper_output_add_plane(pepper_output_t *output, pepper_plane_t *above)
{
	pepper_plane_t *plane;

	PEPPER_CHECK(!above ||
		     above->output == output, return NULL, "Output mismatch.\n");

	plane = (pepper_plane_t *)pepper_object_alloc(PEPPER_OBJECT_PLANE,
			sizeof(pepper_plane_t));
	PEPPER_CHECK(plane, return NULL, "pepper_object_alloc() failed.\n");

	plane->output = output;
	plane->link.item = plane;

	if (above)
		pepper_list_insert(above->link.prev, &plane->link);
	else
		pepper_list_insert(output->plane_list.prev, &plane->link);

	pepper_list_init(&plane->entry_list);
	pixman_region32_init(&plane->damage_region);
	pixman_region32_init(&plane->clip_region);

	return plane;
}

/**
 * Destroy the plane.
 *
 * @param plane     plane to destroy
 */
PEPPER_API void
pepper_plane_destroy(pepper_plane_t *plane)
{
	pepper_plane_entry_t *entry;

	pepper_object_fini(&plane->base);

	pepper_list_for_each(entry, &plane->entry_list, link)
	pepper_view_assign_plane(entry->base.view, plane->output, NULL);

	pepper_list_remove(&plane->link);
	pixman_region32_fini(&plane->damage_region);
	pixman_region32_fini(&plane->clip_region);

	free(plane);
}

void
pepper_plane_add_damage_region(pepper_plane_t *plane, pixman_region32_t *damage)
{
	if (!damage) {
		pixman_region32_union_rect(&plane->damage_region, &plane->damage_region,
					   0, 0, plane->output->geometry.w, plane->output->geometry.h);
		pepper_output_schedule_repaint(plane->output);
	} else if (pixman_region32_not_empty(damage)) {
		pixman_region32_union(&plane->damage_region, &plane->damage_region, damage);
		pepper_output_schedule_repaint(plane->output);
	}
}

/**
 * Get the region that has been changed. Not necessarily the damage region should be visible.
 *
 * @param plane     plane to get the damage region
 *
 * @returns         #pixman_region32_t
 */
PEPPER_API pixman_region32_t *
pepper_plane_get_damage_region(pepper_plane_t *plane)
{
	return &plane->damage_region;
}

/**
 * Get the region that is obscured by other planes in front of the plane. Visible damage region
 * should be (DAMAGE - CLIP)
 *
 * @param plane     plane to get the clip region
 *
 * @returns         #pixman_region32_t
 */
PEPPER_API pixman_region32_t *
pepper_plane_get_clip_region(pepper_plane_t *plane)
{
	return &plane->clip_region;
}

/**
 * Get list of #pepper_render_item_t.
 *
 * @param plane     plane to get the list
 *
 * @returns         #pepper_list_t
 */
PEPPER_API const pepper_list_t *
pepper_plane_get_render_list(pepper_plane_t *plane)
{
	return &plane->entry_list;
}

/**
 * Subtract given region from the damage region of a plane. Called to partially update the
 * damage region of a plane.
 *
 * @param plane     plane
 * @param damage    region to subtract
 */
PEPPER_API void
pepper_plane_subtract_damage_region(pepper_plane_t *plane,
				    pixman_region32_t *damage)
{
	pixman_region32_subtract(&plane->damage_region, &plane->damage_region, damage);
}

/**
 * Clear the damage region of a plane. Called when the output backend has processed the damage
 * region. Or if you partially updated the damage region use pepper_plane_subtract_damage_region.
 *
 * @param plane     plane to clear the damage region
 */
PEPPER_API void
pepper_plane_clear_damage_region(pepper_plane_t *plane)
{
	pixman_region32_clear(&plane->damage_region);
}
