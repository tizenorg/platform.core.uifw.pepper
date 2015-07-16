lib_LTLIBRARIES = libpepper-x11.la
include_HEADERS = pepper-x11.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-x11.pc

libpepper_x11_la_CFLAGS = $(PEPPER_X11_CFLAGS) -Wall
libpepper_x11_la_LIBADD = $(PEPPER_X11_LIBS)

libpepper_x11_la_SOURCES =  pepper-x11.h    \
                            x11-internal.h  \
                            x11-common.c    \
                            x11-output.c    \
                            x11-input.c
