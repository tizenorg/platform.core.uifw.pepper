lib_LTLIBRARIES = libpepper-drm.la
include_HEADERS = pepper-drm.h

libpepper_drm_la_CFLAGS = $(PEPPER_DRM_CFLAGS)                  \
                          $(PEPPER_INTERNAL_CFLAGS)             \
                          $(PEPPER_RENDER_INTERNAL_CFLAGS)      \
                          $(PEPPER_LIBINPUT_INTERNAL_CFLAGS)

libpepper_drm_la_LIBADD = $(PEPPER_DRM_LIBS)                \
                          $(PEPPER_INTERNAL_LIBS)           \
                          $(PEPPER_RENDER_INTERNAL_LIBS)    \
                          $(PEPPER_LIBINPUT_INTERNAL_LIBS)

libpepper_drm_la_SOURCES =  drm-internal.h  \
                            drm-common.c    \
                            drm-output.c
