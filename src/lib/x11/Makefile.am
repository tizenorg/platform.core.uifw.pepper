lib_LTLIBRARIES = libpepper-x11.la
include_HEADERS = pepper-x11.h

libpepper_x11_la_CFLAGS = $(X11_CFLAGS)                     \
                          -I$(top_srcdir)/src/lib/pepper    \
                          -I$(top_srcdir)/src/lib/render    \
                          $(PIXMAN_CFLAGS)

libpepper_x11_la_LIBADD = $(X11_LIBS)                                       \
                          $(top_srcdir)/src/lib/pepper/libpepper.la         \
                          $(top_srcdir)/src/lib/render/libpepper-render.la

libpepper_x11_la_SOURCES =  pepper-x11.h    \
                            x11-internal.h  \
                            x11-common.c    \
                            x11-output.c    \
                            x11-input.c
