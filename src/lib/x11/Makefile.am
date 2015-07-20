lib_LTLIBRARIES = libpepper-x11.la
include_HEADERS = pepper-x11.h

libpepper_x11_la_CFLAGS = $(PEPPER_X11_CFLAGS)              \
                          $(PEPPER_INTERNAL_CFLAGS)         \
                          $(PEPPER_RENDER_INTERNAL_CFLAGS)

libpepper_x11_la_LIBADD = $(PEPPER_X11_LIBS)                \
                          $(PEPPER_INTERNAL_LIBS)           \
                          $(PEPPER_RENDER_INTERNAL_LIBS)

libpepper_x11_la_SOURCES =  pepper-x11.h    \
                            x11-internal.h  \
                            x11-common.c    \
                            x11-output.c    \
                            x11-input.c
