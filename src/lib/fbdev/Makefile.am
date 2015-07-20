lib_LTLIBRARIES = libpepper-fbdev.la
include_HEADERS = pepper-fbdev.h

libpepper_fbdev_la_CFLAGS = $(PEPPER_INTERNAL_CFLAGS)           \
                            $(PEPPER_RENDER_INTERNAL_CFLAGS)    \
                            $(PEPPER_LIBINPUT_INTERNAL_CFLAGS)

libpepper_fbdev_la_LIBADD = $(PEPPER_INTERNAL_LIBS)             \
                            $(PEPPER_RENDER_INTERNAL_LIBS)      \
                            $(PEPPER_LIBINPUT_INTERNAL_LIBS)

libpepper_fbdev_la_SOURCES = fbdev-internal.h   \
                             fbdev-common.c     \
                             fbdev-output.c
