lib_LTLIBRARIES = libpepper-fbdev.la

libpepper_fbdev_includedir=$(includedir)/pepper
libpepper_fbdev_include_HEADERS = pepper-fbdev.h

libpepper_fbdev_la_CFLAGS = $(PEPPER_FBDEV_CFLAGS)
libpepper_fbdev_la_LIBADD = $(PEPPER_FBDEV_LIBS)

libpepper_fbdev_la_SOURCES = fbdev-internal.h   \
                             fbdev-common.c     \
                             fbdev-output.c
