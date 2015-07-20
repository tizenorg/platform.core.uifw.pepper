lib_LTLIBRARIES = libpepper-wayland.la
include_HEADERS = pepper-wayland.h

libpepper_wayland_la_CFLAGS = $(PEPPER_WAYLAND_CFLAGS)          \
                              $(PEPPER_INTERNAL_CFLAGS)         \
                              $(PEPPER_RENDER_INTERNAL_CFLAGS)

libpepper_wayland_la_LIBADD = $(PEPPER_WAYLAND_LIBS)            \
                              $(PEPPER_INTERNAL_LIBS)           \
                              $(PEPPER_RENDER_INTERNAL_LIBS)

libpepper_wayland_la_SOURCES =  pepper-wayland.h        \
                                wayland-internal.h      \
                                wayland-common.c        \
                                wayland-output.c        \
                                wayland-input.c         \
                                wayland-shm-buffer.c
