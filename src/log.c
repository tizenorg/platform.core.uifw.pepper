#include <stdio.h>

#include <wayland-util.h>
#include "debug_ch.h"

DECLARE_DEBUG_CHANNEL(libwayland);

static void
pepper_wayland_log_func(const char* format, va_list args)
{
#if 0
    fprintf(stderr, "libwayland: ");
    vfprintf(stderr, format, args);
#else
    ERR(format, args);
#endif
}

void
pepper_log_init(const char* filename)
{
    wl_log_set_handler_server(pepper_wayland_log_func);
    dbg_set_logfile(filename);
}
