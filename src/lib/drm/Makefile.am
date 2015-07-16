lib_LTLIBRARIES = libpepper-drm.la
include_HEADERS = pepper-drm.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-drm.pc

libpepper_drm_la_CFLAGS = $(PEPPER_DRM_CFLAGS) -Wall       \
                          -I$(top_srcdir)/src/lib/pepper   \
                          -I$(top_srcdir)/src/lib/libinput \
                          -I$(top_srcdir)/src/lib/render 
libpepper_drm_la_LIBADD = $(PEPPER_DRM_LIBS)                                   \
                          $(top_srcdir)/src/lib/pepper/libpepper.la            \
                          $(top_srcdir)/src/lib/libinput/libpepper-libinput.la \
                          $(top_srcdir)/src/lib/render/libpepper-render.la

libpepper_drm_la_SOURCES =  drm-internal.h      \
                            drm-common.c        \
                            drm-output.c
