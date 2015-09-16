lib_LTLIBRARIES = libpepper.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_includedir=$(includedir)/pepper
libpepper_include_HEADERS = pepper.h pepper-utils.h pepper-output-backend.h pepper-input-backend.h

libpepper_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_CFLAGS)
libpepper_la_LIBADD = $(PEPPER_LIBS) -lm

libpepper_la_SOURCES = pepper.h                 \
                       pepper-internal.h        \
                       object.c                 \
                       compositor.c             \
                       output.c                 \
                       input.c                  \
                       pointer.c                \
                       keyboard.c               \
                       touch.c                  \
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
