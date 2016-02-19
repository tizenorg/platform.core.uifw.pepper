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

#include <config.h>
#include <libudev.h>
#include <fcntl.h>
#include <unistd.h>
#include "drm-internal.h"
#ifdef HAVE_DRM_SPRD
#include <sprd_drmif.h>
struct sprd_drm_device *sprd_dev = NULL;
#endif

static struct udev_device *
find_primary_gpu(struct udev *udev) /* FIXME: copied from weston */
{
	struct udev_enumerate   *e;
	struct udev_list_entry  *entry;
	struct udev_device      *pci, *device, *drm_device = NULL;
	const char              *path, *device_seat, *id;

	e = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(e, "drm");
	udev_enumerate_add_match_sysname(e, "card[0-9]*");
	udev_enumerate_scan_devices(e);

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e)) {
		path = udev_list_entry_get_name(entry);
		device = udev_device_new_from_syspath(udev, path);
		if (!device)
			continue;
		device_seat = udev_device_get_property_value(device, "ID_SEAT");
		if (!device_seat)
			device_seat = "seat0";
		if (strcmp(device_seat, "seat0" /* FIXME: default seat? */)) {
			udev_device_unref(device);
			continue;
		}

		pci = udev_device_get_parent_with_subsystem_devtype(device, "pci", NULL);
		if (pci) {
			id = udev_device_get_sysattr_value(pci, "boot_vga");
			if (id && !strcmp(id, "1")) {
				if (drm_device)
					udev_device_unref(drm_device);
				drm_device = device;
				break;
			}
		}

		if (!drm_device)
			drm_device = device;
		else
			udev_device_unref(device);
	}

	udev_enumerate_unref(e);
	return drm_device;
}

#ifdef HAVE_DRM_SPRD
static void
drm_sprd_init(int fd)
{
	drmVersionPtr drm_info;
	int drmIRQ = 78;
	int length = 0;

	if (sprd_dev) {
		return;
	}

	drm_info = drmGetVersion(fd);
	length = drm_info->name_len;

	if (length != 4) {
		drmFreeVersion(drm_info);
		return;
	}
	if (strncmp("sprd", drm_info->name, 4)) {
		drmFreeVersion(drm_info);
		return;
	}
	drmFreeVersion(drm_info);

	PEPPER_CHECK(!drmCtlInstHandler(fd, drmIRQ), return,
	             "drmCtlInstHandler() failed.\n");

	sprd_dev = sprd_device_create(fd);
	PEPPER_CHECK(sprd_dev, return, "sprd_device_create() failed.\n");

	return;
}

static void
drm_sprd_deinit(void)
{
	if (sprd_dev == NULL)
		return;

	sprd_device_destroy(sprd_dev);
	sprd_dev = NULL;
}
#endif

static int
handle_drm_event(int fd, uint32_t mask, void *data)
{
	pepper_drm_t *drm = data;
	drmHandleEvent(fd, &drm->drm_event_context);
	return 0;
}

static int
handle_udev_event(int fd, uint32_t mask, void *data)
{
	pepper_drm_t *drm = (pepper_drm_t *)data;
	struct udev_device *device;

	const char *sysnum;
	const char *value;

	device = udev_monitor_receive_device(drm->udev_monitor);

	sysnum = udev_device_get_sysnum(device);
	if (!sysnum || atoi(sysnum) != drm->sysnum)
		goto done;

	value = udev_device_get_property_value(device, "HOTPLUG");
	if (!value || strcmp(value, "1"))
		goto done;

	drm_update_connectors(drm);

done:
	udev_device_unref(device);
	return 0;
}

PEPPER_API pepper_drm_t *
pepper_drm_create(pepper_compositor_t *compositor, struct udev *udev,
                  const char *device)
{
	pepper_drm_t           *drm = NULL;
	struct udev_device     *udev_device = NULL;
	const char             *sysnum_str = NULL;
	const char             *filepath;
	struct stat             s;
	int                     ret;
	struct wl_event_loop   *loop;
	drm_magic_t             magic;
	uint64_t                cap;
	clockid_t               clock_id;

	drm = calloc(1, sizeof(pepper_drm_t));
	PEPPER_CHECK(drm, goto error, "calloc() failed.\n");

	drm->compositor = compositor;
	drm->fd = -1;
	pepper_list_init(&drm->plane_list);
	pepper_list_init(&drm->connector_list);

	/* Find primary GPU udev device. Usually card0. */
	udev_device = find_primary_gpu(udev);
	PEPPER_CHECK(udev_device, goto error, "find_primary_gpu() failed.\n");

	/* Get sysnum for the device. */
	sysnum_str = udev_device_get_sysnum(udev_device);
	PEPPER_CHECK(sysnum_str, goto error, "udev_device_get_sysnum() failed.\n");

	drm->sysnum = atoi(sysnum_str);
	PEPPER_CHECK(drm->sysnum >= 0, goto error, "Invalid sysnum.\n");

	/* Get device file path. */
	filepath = udev_device_get_devnode(udev_device);
	PEPPER_CHECK(filepath, goto error, "udev_device_get_devnode() failed.\n");

	/* Open DRM device file and check validity. */
	drm->fd = open(filepath, O_RDWR | O_CLOEXEC);
	PEPPER_CHECK(drm->fd != -1, goto error,
	             "open(%s, O_RDWR | O_CLOEXEC) failed.\n", filepath);

#ifdef HAVE_DRM_SPRD
	/*If drm is sprd, this fuction MUST call before other drm function*/
	drm_sprd_init(drm->fd);
#endif

	ret = fstat(drm->fd, &s);
	PEPPER_CHECK(ret != -1, goto error, "fstat() failed %s.\n", filepath);

	ret = major(s.st_rdev);
	PEPPER_CHECK(ret == 226, goto error, "Not a drm device file %s.\n", filepath);

	ret = drmGetMagic(drm->fd, &magic);
	PEPPER_CHECK(ret == 0, goto error, "drmGetMagic() failed.\n");

	ret = drmAuthMagic(drm->fd, magic);
	PEPPER_CHECK(ret == 0, goto error, "drmAuthMagic() failed.\n");

	/* Create udev event monitor. */
	drm->udev_monitor = udev_monitor_new_from_netlink(udev, "udev");
	PEPPER_CHECK(drm->udev_monitor, goto error,
	             "udev_monitor_new_from_netlink() failed.\n");
	udev_monitor_filter_add_match_subsystem_devtype(drm->udev_monitor, "drm", NULL);

#ifdef HAVE_TBM
	/* Create wayland-tbm*/
	drm->wl_tbm_server = wayland_tbm_server_init(pepper_compositor_get_display(
	                             compositor),
	                     filepath, drm->fd, 0);
#endif

	/* Add DRM event FDs to the compositor's display. */
	loop = wl_display_get_event_loop(pepper_compositor_get_display(compositor));

	drm->drm_event_source = wl_event_loop_add_fd(loop, drm->fd, WL_EVENT_READABLE,
	                        handle_drm_event, drm);
	PEPPER_CHECK(drm->drm_event_source, goto error,
	             "wl_event_loop_add() failed.\n");

	drm->udev_event_source = wl_event_loop_add_fd(loop,
	                         udev_monitor_get_fd(drm->udev_monitor),
	                         WL_EVENT_READABLE, handle_udev_event, drm);
	PEPPER_CHECK(drm->udev_event_source, goto error,
	             "wl_event_loop_add() failed.\n");

	drm->drm_event_context.version = DRM_EVENT_CONTEXT_VERSION;
	drm->drm_event_context.vblank_handler = drm_handle_vblank;
	drm->drm_event_context.page_flip_handler = drm_handle_page_flip;

	ret = udev_monitor_enable_receiving(drm->udev_monitor);
	PEPPER_CHECK(ret >= 0, goto error, "udev_monitor_enable_receiving() failed.\n");

	drm->resources = drmModeGetResources(drm->fd);
	PEPPER_CHECK(drm->resources, goto error, "drmModeGetResources() failed.\n");

	ret = drmGetCap(drm->fd, 0x8 /* DRM_CAP_CURSOR_WIDTH */, &cap);
	drm->cursor_width = (ret == 0) ? cap : 64;

	ret = drmGetCap(drm->fd, 0x9 /* DRM_CAP_CURSOR_HEIGHT */, &cap);
	drm->cursor_height = (ret == 0) ? cap : 64;

	drm_init_planes(drm);
	drm_init_connectors(drm);
	udev_device_unref(udev_device);

	/* Try to set clock. */
	ret = drmGetCap(drm->fd, DRM_CAP_TIMESTAMP_MONOTONIC, &cap);

	if (ret == 0 && cap == 1)
		clock_id = CLOCK_MONOTONIC;
	else
		clock_id = CLOCK_REALTIME;

	if (!pepper_compositor_set_clock_id(compositor, clock_id))
		goto error;

	return drm;

error:
	if (drm)
		pepper_drm_destroy(drm);

	if (udev_device)
		udev_device_unref(udev_device);

	return NULL;
}

PEPPER_API void
pepper_drm_destroy(pepper_drm_t *drm)
{
	drm_connector_t *conn, *next_conn;
	drm_plane_t     *plane, *next_plane;

	pepper_list_for_each_safe(conn, next_conn, &drm->connector_list, link)
	drm_connector_destroy(conn);

	pepper_list_for_each_safe(plane, next_plane, &drm->plane_list, link)
	drm_plane_destroy(plane);

	if (drm->pixman_renderer)
		pepper_renderer_destroy(drm->pixman_renderer);

	if (drm->gl_renderer)
		pepper_renderer_destroy(drm->gl_renderer);

#ifdef HAVE_TBM
	if (drm->wl_tbm_server)
		wayland_tbm_server_deinit(drm->wl_tbm_server);
#endif

	if (drm->gbm_device)
		gbm_device_destroy(drm->gbm_device);

	if (drm->resources)
		drmModeFreeResources(drm->resources);

	if (drm->udev_event_source)
		wl_event_source_remove(drm->udev_event_source);

	if (drm->drm_event_source)
		wl_event_source_remove(drm->drm_event_source);

	if (drm->udev_monitor)
		udev_monitor_unref(drm->udev_monitor);

#ifdef HAVE_DRM_SPRD
	drm_sprd_deinit();
#endif

	if (drm->fd != -1)
		close(drm->fd);

	free(drm);
}
