lib_LTLIBRARIES = libpepper-wayland.la

libpepper_wayland_includedir=$(includedir)/pepper
libpepper_wayland_include_HEADERS = pepper-wayland.h

libpepper_wayland_la_CFLAGS = $(PEPPER_WAYLAND_CFLAGS)
libpepper_wayland_la_LIBADD = $(PEPPER_WAYLAND_LIBS)

libpepper_wayland_la_SOURCES =  pepper-wayland.h        \
                                wayland-internal.h      \
                                wayland-common.c        \
                                wayland-output.c        \
                                wayland-input.c         \
                                wayland-shm-buffer.c
