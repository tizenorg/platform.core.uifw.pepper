lib_LTLIBRARIES = libpepper-fbdev.la
include_HEADERS = pepper-fbdev.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-fbdev.pc

libpepper_fbdev_la_CFLAGS = $(PEPPER_FBDEV_CFLAGS) -Wall     \
                            -I$(top_srcdir)/src/lib/pepper   \
                            -I$(top_srcdir)/src/lib/libinput \
                            -I$(top_srcdir)/src/lib/render
libpepper_fbdev_la_LIBADD = $(PEPPER_FBDEV_LIBS)                                 \
                            $(top_srcdir)/src/lib/pepper/libpepper.la            \
                            $(top_srcdir)/src/lib/libinput/libpepper-libinput.la \
                            $(top_srcdir)/src/lib/render/libpepper-render.la
libpepper_fbdev_la_SOURCES = fbdev-internal.h      \
                             fbdev-common.c        \
                             fbdev-output.c
