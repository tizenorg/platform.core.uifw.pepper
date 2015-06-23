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

#define PEPPER_ERROR(...)   /* TODO */

static struct _vt_data
{
    int     tty_fd;
    int     tty_num;
    int     saved_tty_num;
    int     kb_mode;
} vt_data;

PEPPER_API void
pepper_virtual_terminal_restore()
{
    if (vt_data.tty_fd >= 0)
    {
        int             fd = vt_data.tty_fd;
        struct vt_mode  mode = {0};

        if ((vt_data.kb_mode >= 0) && (ioctl(fd, KDSKBMODE, vt_data.kb_mode) < 0))
            PEPPER_ERROR("");

        if (ioctl(fd, KDSETMODE, KD_TEXT) < 0)
            PEPPER_ERROR("");

        mode.mode = VT_AUTO;
        if (ioctl(fd, VT_SETMODE, &mode) < 0)
            PEPPER_ERROR("");

        if ((vt_data.saved_tty_num > 0) && (ioctl(fd, VT_ACTIVATE, vt_data.saved_tty_num) < 0))
            PEPPER_ERROR("");

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

    if (tty == 0)
    {
        fd = dup(tty);
        if (fd < 0)
            return PEPPER_FALSE;

        if (ioctl(fd, VT_GETSTATE, &stat) == 0)
            vt_data.tty_num = vt_data.saved_tty_num = stat.v_active;
    }
    else
    {
        char tty_str[32];

        sprintf(tty_str, "/dev/tty%d", tty);
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
