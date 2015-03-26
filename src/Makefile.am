bin_PROGRAMS =
lib_LTLIBRARIES =
include_HEADERS =
noinst_LTLIBRARIES =

# pepper library
lib_LTLIBRARIES += libpepper.la
include_HEADERS += pepper.h

libpepper_la_CFLAGS = $(LIB_PEPPER_CFLAGS)
libpepper_la_LIBADD = $(LIB_PEPPER_LIBS)

libpepper_la_SOURCES = pepper.h                 \
                       pepper_internal.h        \
                       common.h                 \
                       common.c                 \
                       compositor.c             \
                       output.c                 \
                       input.c                  \
                       client.c                 \
                       shell.c                  \
                       surface.c                \
                       region.c

# wayland backend
if ENABLE_WAYLAND_BACKEND
include_HEADERS += modules/wayland/pepper-wayland.h

libpepper_la_CFLAGS += $(WAYLAND_BACKEND_CFLAGS)
libpepper_la_LIBADD += $(WAYLAND_BACKEND_LIBS)

libpepper_la_SOURCES += modules/wayland/wayland-internal.h  \
                        modules/wayland/wayland-common.c    \
                        modules/wayland/wayland-output.c    \
                        modules/wayland/wayland-input.c
endif

# drm backend
if ENABLE_DRM_BACKEND
include_HEADERS += modules/drm/pepper-drm.h

libpepper_la_CFLAGS += $(DRM_BACKEND_CFLAGS)
libpepper_la_LIBADD += $(DRM_BACKEND_LIBS)

libpepper_la_SOURCES += modules/drm/drm-internal.h
endif

# Pepper server executable
if ENABLE_PEPPER_SERVER
bin_PROGRAMS += pepper

pepper_LDFLAGS = -export-dynamic
pepper_CFLAGS = $(PEPPER_SERVER_CFLAGS) -I$(top_srcdir)/src
pepper_LDADD = $(PEPPER_SERVER_LIBS) -ldl libpepper.la

pepper_SOURCES = server/server.c
endif
