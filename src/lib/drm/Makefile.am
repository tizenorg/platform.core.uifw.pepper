lib_LTLIBRARIES = libpepper-drm.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_drm_includedir=$(includedir)/pepper
libpepper_drm_include_HEADERS = pepper-drm.h

libpepper_drm_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_DRM_CFLAGS)
libpepper_drm_la_LIBADD = $(PEPPER_DRM_LIBS)

libpepper_drm_la_SOURCES =  drm-internal.h  \
                            drm-common.c    \
                            drm-output.c
