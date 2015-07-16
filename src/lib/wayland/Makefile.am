lib_LTLIBRARIES = libpepper-wayland.la
include_HEADERS = pepper-wayland.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-wayland.pc

libpepper_wayland_la_CFLAGS = $(PEPPER_WAYLAND_CFLAGS) -Wall \
                              -I$(top_srcdir)/src/lib/pepper \
                              -I$(top_srcdir)/src/lib/render
libpepper_wayland_la_LIBADD = $(PEPPER_WAYLAND_LIBS)                    \
                              $(top_srcdir)/src/lib/pepper/libpepper.la \
                              $(top_srcdir)/src/lib/render/libpepper-render.la
libpepper_wayland_la_SOURCES =  pepper-wayland.h        \
                                wayland-internal.h      \
                                wayland-common.c        \
                                wayland-output.c        \
                                wayland-input.c         \
                                wayland-shm-buffer.c
