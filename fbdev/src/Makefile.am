lib_LTLIBRARIES = libpepper-fbdev.la
include_HEADERS = pepper-fbdev.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-fbdev.pc

libpepper_fbdev_la_CFLAGS = $(PEPPER_FBDEV_CFLAGS) -Wall
libpepper_fbdev_la_LIBADD = $(PEPPER_FBDEV_LIBS)

libpepper_fbdev_la_SOURCES = fbdev-internal.h      \
                             fbdev-common.c        \
                             fbdev-output.c
