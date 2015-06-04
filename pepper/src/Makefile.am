lib_LTLIBRARIES = libpepper.la
include_HEADERS = pepper.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper.pc

libpepper_la_CFLAGS = $(PEPPER_CFLAGS) -Wall
libpepper_la_LIBADD = $(PEPPER_LIBS)

libpepper_la_SOURCES = pepper.h                 \
                       pepper-internal.h        \
                       common.h                 \
                       common.c                 \
                       compositor.c             \
                       output.c                 \
                       input.c                  \
                       surface.c                \
                       region.c                 \
                       buffer.c                 \
                       data-device.c            \
                       view.c                   \
                       pepper-util.h            \
                       pepper-util.c
