lib_LTLIBRARIES = libpepper.la
include_HEADERS = pepper.h pepper-utils.h pepper-output-backend.h pepper-input-backend.h

libpepper_la_CFLAGS = $(WAYLAND_SERVER_CFLAGS) $(PIXMAN_CFLAGS)
libpepper_la_LIBADD = $(WAYLAND_SERVER_LIBS) $(PIXMAN_LIBS) -lm

libpepper_la_SOURCES = pepper.h                 \
                       pepper-internal.h        \
                       common.h                 \
                       common.c                 \
                       object.c                 \
                       compositor.c             \
                       output.c                 \
                       input.c                  \
                       surface.c                \
                       region.c                 \
                       buffer.c                 \
                       data-device.c            \
                       view.c                   \
                       plane.c                  \
                       utils-file.c             \
                       utils-map.c              \
                       utils-log.c              \
                       utils-vt.c
