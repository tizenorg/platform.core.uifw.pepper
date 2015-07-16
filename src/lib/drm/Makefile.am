lib_LTLIBRARIES = libpepper-drm.la
include_HEADERS = pepper-drm.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-drm.pc

libpepper_drm_la_CFLAGS = $(PEPPER_DRM_CFLAGS) -Wall
libpepper_drm_la_LIBADD = $(PEPPER_DRM_LIBS)

libpepper_drm_la_SOURCES =  drm-internal.h      \
                            drm-common.c        \
                            drm-output.c
