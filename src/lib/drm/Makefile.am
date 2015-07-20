lib_LTLIBRARIES = libpepper-drm.la
include_HEADERS = pepper-drm.h

libpepper_drm_la_CFLAGS = $(LIBDRM_CFLAGS)                  \
                          -I$(top_srcdir)/src/lib/pepper    \
                          -I$(top_srcdir)/src/lib/libinput  \
                          -I$(top_srcdir)/src/lib/render    \
                          $(PIXMAN_CFLAGS)

libpepper_drm_la_LIBADD = $(LIBDRM_LIBS)                                        \
                          $(top_srcdir)/src/lib/pepper/libpepper.la             \
                          $(top_srcdir)/src/lib/libinput/libpepper-libinput.la  \
                          $(top_srcdir)/src/lib/render/libpepper-render.la

libpepper_drm_la_SOURCES =  drm-internal.h  \
                            drm-common.c    \
                            drm-output.c
