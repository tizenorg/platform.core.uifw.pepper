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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pepper-utils.h"

static struct _vt_data {
	int     tty_fd;
	int     tty_num;
	int     saved_tty_num;
	int     kb_mode;
} vt_data;

PEPPER_API void
pepper_virtual_terminal_restore(void)
{
	if (vt_data.tty_fd >= 0) {
		int             fd = vt_data.tty_fd;
		struct vt_mode  mode = {0};

		if ((vt_data.kb_mode >= 0) && (ioctl(fd, KDSKBMODE, vt_data.kb_mode) < 0)) {
			PEPPER_ERROR("");
		}

		if (ioctl(fd, KDSETMODE, KD_TEXT) < 0) {
			PEPPER_ERROR("");
		}

		mode.mode = VT_AUTO;
		if (ioctl(fd, VT_SETMODE, &mode) < 0) {
			PEPPER_ERROR("");
		}

		if ((vt_data.saved_tty_num > 0) &&
			(ioctl(fd, VT_ACTIVATE, vt_data.saved_tty_num) < 0)) {
			PEPPER_ERROR("");
		}

		close(fd);
	}
}

PEPPER_API pepper_bool_t
pepper_virtual_terminal_setup(int tty)
{
	int fd;

	struct vt_stat  stat;
	struct vt_mode  mode;

	memset(&vt_data, -1, sizeof(vt_data));

	if (tty == 0) {
		fd = dup(tty);
		if (fd < 0)
			return PEPPER_FALSE;

		if (ioctl(fd, VT_GETSTATE, &stat) == 0)
			vt_data.tty_num = vt_data.saved_tty_num = stat.v_active;
	} else {
		char tty_str[32];

		snprintf(tty_str, 32, "/dev/tty%d", tty);
		fd = open(tty_str, O_RDWR | O_CLOEXEC);
		if (fd < 0)
			return PEPPER_FALSE;

		if (ioctl(fd, VT_GETSTATE, &stat) == 0)
			vt_data.saved_tty_num = stat.v_active;

		vt_data.tty_num = tty;
	}

	vt_data.tty_fd = fd;

	if (ioctl(fd, VT_ACTIVATE, vt_data.tty_num) < 0)
		goto error;

	if (ioctl(fd, VT_WAITACTIVE, vt_data.tty_num) < 0)
		goto error;

	if (ioctl(fd, KDGKBMODE, &vt_data.kb_mode) < 0)
		goto error;

	if (ioctl(fd, KDSKBMODE, K_OFF) < 0)
		goto error;

	if (ioctl(fd, KDSETMODE, KD_GRAPHICS) < 0)
		goto error;

	if (ioctl(fd, VT_GETMODE, &mode) < 0)
		goto error;

	mode.mode = VT_PROCESS;
	mode.relsig = SIGUSR1;
	mode.acqsig = SIGUSR1;
	if (ioctl(fd, VT_SETMODE, &mode) < 0)
		goto error;

	/* TODO: add signal handling code for SIGUSR1 */

	return PEPPER_TRUE;

error:

	pepper_virtual_terminal_restore();

	return PEPPER_FALSE;
}
