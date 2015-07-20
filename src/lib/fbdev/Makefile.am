lib_LTLIBRARIES = libpepper-fbdev.la
include_HEADERS = pepper-fbdev.h

libpepper_fbdev_la_CFLAGS = -I$(top_srcdir)/src/lib/pepper      \
                            -I$(top_srcdir)/src/lib/libinput    \
                            -I$(top_srcdir)/src/lib/render      \
                            $(PIXMAN_CFLAGS)                    \
                            $(WAYLAND_SERVER_CFLAGS)

libpepper_fbdev_la_LIBADD = $(top_srcdir)/src/lib/pepper/libpepper.la               \
                            $(top_srcdir)/src/lib/libinput/libpepper-libinput.la    \
                            $(top_srcdir)/src/lib/render/libpepper-render.la

libpepper_fbdev_la_SOURCES = fbdev-internal.h   \
                             fbdev-common.c     \
                             fbdev-output.c
