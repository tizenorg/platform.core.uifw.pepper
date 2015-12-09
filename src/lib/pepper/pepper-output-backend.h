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

#ifndef PEPPER_OUTPUT_BACKEND_H
#define PEPPER_OUTPUT_BACKEND_H

#include <pepper.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef pepper_plane_t
 *
 * A #pepper_plane_t is a separate layer of an output stacked from bottom to top.
 */
typedef struct pepper_plane             pepper_plane_t;

/**
 * @typedef pepper_output_backend_t
 *
 * A #pepper_output_backend_t is a set of interface functions to use output backend.
 */
typedef struct pepper_output_backend    pepper_output_backend_t;

/**
 * @typedef pepper_render_item_t
 *
 * A #pepper_render_item_t is a set of rendering information of a view.
 */
typedef struct pepper_render_item       pepper_render_item_t;

struct pepper_output_backend
{
    /**
     * Destroy all internal resources by the backend for the given output.
     */
    void            (*destroy)(void *output);

    /**
     * Return the sub-pixel layout of the physical display screen.
     */
    int32_t         (*get_subpixel_order)(void *output);

    /**
     * Return a string indicating maker of the output device.
     */
    const char *    (*get_maker_name)(void *output);

    /**
     * Return a string indicating model name of the output device.
     */
    const char *    (*get_model_name)(void *output);

    /**
     * Return the number of available modes on the output device.
     */
    int             (*get_mode_count)(void *output);

    /**
     * Return mode info for the given mode index.
     */
    void            (*get_mode)(void *output, int index, pepper_output_mode_t *mode);

    /**
     * Change output mode to the given mode. return PEPPER_TRUE on success.
     */
    pepper_bool_t   (*set_mode)(void *output, const pepper_output_mode_t *mode);

    /**
     * Assign plane for each views in the view list. Called before repaint. Backend should
     * assign planes for the views in the given view list.
     */
    void            (*assign_planes)(void *output, const pepper_list_t *view_list);

    /**
     * Start repaint. Calld when the repaint state has been changed from idle to scheduled.
     * Backend should call pepper_output_finish_frame() when ready.
     */
    void            (*start_repaint_loop)(void *output);

    /**
     * Render planes in the plane list. Backend should repaint the output. Backend should call
     * pepper_output_finish_frame() when it is ready to draw a new frame.
     */
    void            (*repaint)(void *output, const pepper_list_t *plane_list);

    /**
     * Attach buffer to the surface. Called immediately when a new buffer has been attached and
     * committed. Backend must return the size of the attached buffer. Backend have a chance to
     * allocate resources for the surface.
     */
    void            (*attach_surface)(void *output, pepper_surface_t *surface, int *w, int *h);

    /**
     * Update the surface content. Called before repaint. Backend should handle damage region
     * of the given surface if it is maintaining a copy of the surface.
     */
    void            (*flush_surface_damage)(void *output, pepper_surface_t *surface,
                                            pepper_bool_t *keep_buffer);
};

/**
 * Create and add #pepper_output_t to compositor.
 *
 * @param compositor    compositor to add the output
 * @param backend       pointer to an output backend function table
 * @param name          name of the output to add
 * @param data          backend data
 * @param transform     transform of the output to add
 * @param scale         scale of the output to add
 *
 * @returns             #pepper_output_t
 */
PEPPER_API pepper_output_t *
pepper_compositor_add_output(pepper_compositor_t *compositor,
                             const pepper_output_backend_t *backend, const char *name, void *data,
                             int transform, int scale);

struct pepper_render_item
{
    pepper_view_t       *view;          /**< view to render */
    pepper_mat4_t       transform;      /**< transform matrix of the view */
    pepper_mat4_t       inverse;        /**< inverse matrix of the transform matrix */
    pixman_region32_t   visible_region; /**< visible region of the view */
};

/**
 * Create and add #pepper_plane_t to the output.
 *
 * @param output        output to add the plane
 * @param above_plane   added plane will be placed above above_plane
 *
 * @returns             #pepper_plane_t
 */
PEPPER_API pepper_plane_t *
pepper_output_add_plane(pepper_output_t *output, pepper_plane_t *above_plane);

/**
 * Destroy the plane.
 *
 * @param plane     plane to destroy
 */
PEPPER_API void
pepper_plane_destroy(pepper_plane_t *plane);

/**
 * Get the region that has been changed. Not necessarily the damage region should be visible.
 *
 * @param plane     plane to get the damage region
 *
 * @returns         #pixman_region32_t
 */
PEPPER_API pixman_region32_t *
pepper_plane_get_damage_region(pepper_plane_t *plane);

/**
 * Get the region that is obscured by other planes in front of the plane. Visible damage region
 * should be (DAMAGE - CLIP)
 *
 * @param plane     plane to get the clip region
 *
 * @returns         #pixman_region32_t
 */
PEPPER_API pixman_region32_t *
pepper_plane_get_clip_region(pepper_plane_t *plane);

/**
 * Get list of #pepper_render_item_t.
 *
 * @param plane     plane to get the list
 *
 * @returns         #pepper_list_t
 */
PEPPER_API const pepper_list_t *
pepper_plane_get_render_list(pepper_plane_t *plane);

/**
 * Subtract given region from the damage region of a plane. Called to partially update the
 * damage region of a plane.
 *
 * @param plane     plane
 * @param damage    region to subtract
 */
PEPPER_API void
pepper_plane_subtract_damage_region(pepper_plane_t *plane, pixman_region32_t *damage);

/**
 * Clear the damage region of a plane. Called when the output backend has processed the damage
 * region. Or if you partially updated the damage region use pepper_plane_subtract_damage_region.
 *
 * @param plane     plane to clear the damage region
 */
PEPPER_API void
pepper_plane_clear_damage_region(pepper_plane_t *plane);

/**
 * Assign a view to a plane.
 *
 * @param view      view to assign
 * @param output    output of the plane
 * @param plane     plane to assign a view
 */
PEPPER_API void
pepper_view_assign_plane(pepper_view_t *view, pepper_output_t *output, pepper_plane_t *plane);

/**
 * Add damage region to planes in the output. if damage region is empty, whole area is set to
 * damage region.
 *
 * @param output    output to add damage
 * @param region    damage region
 */
PEPPER_API void
pepper_output_add_damage_region(pepper_output_t *output, pixman_region32_t *region);

/**
 * Notifying that the pending frame has been finished. Output backend should call this function
 * when they are ready to draw a new frame in response to the requests from the pepper library.
 *
 * @param output    output to finish frame
 * @param ts        time of finishing frame
 */
PEPPER_API void
pepper_output_finish_frame(pepper_output_t *output, struct timespec *ts);

/**
 * Destroy output and backend internal resources.
 *
 * @param output    output to destroy
 */
PEPPER_API void
pepper_output_remove(pepper_output_t *output);

/**
 * Update output mode. Backend should call this function after change output mode.
 *
 * @param output    output to update mode
 */
PEPPER_API void
pepper_output_update_mode(pepper_output_t *output);

#ifdef __cplusplus
}
#endif

#endif /* PEPPER_OUTPUT_BACKEND_H */
