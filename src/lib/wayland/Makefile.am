lib_LTLIBRARIES = libpepper-wayland.la
include_HEADERS = pepper-wayland.h

libpepper_wayland_la_CFLAGS = $(WAYLAND_CLIENT_CFLAGS)          \
                              -I$(top_srcdir)/src/lib/pepper    \
                              -I$(top_srcdir)/src/lib/render    \
                              $(PIXMAN_CFLAGS)

libpepper_wayland_la_LIBADD = $(WAYLAND_CLIENT_LIBS)                            \
                              $(top_srcdir)/src/lib/pepper/libpepper.la         \
                              $(top_srcdir)/src/lib/render/libpepper-render.la

libpepper_wayland_la_SOURCES =  pepper-wayland.h        \
                                wayland-internal.h      \
                                wayland-common.c        \
                                wayland-output.c        \
                                wayland-input.c         \
                                wayland-shm-buffer.c
