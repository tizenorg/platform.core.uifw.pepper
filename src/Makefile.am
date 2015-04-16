bin_PROGRAMS =
lib_LTLIBRARIES =
include_HEADERS =
noinst_LTLIBRARIES =

# pepper library
lib_LTLIBRARIES += libpepper.la
include_HEADERS += pepper.h pepper-pixman-renderer.h

libpepper_la_CFLAGS = $(LIB_PEPPER_CFLAGS) -Wall
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
                       region.c                 \
                       buffer.c                 \
                       renderer.c               \
                       pixman-renderer.c

# gl renderer
if ENABLE_GL_RENDERER
include_HEADERS += pepper-gl-renderer.h

libpepper_la_CFLAGS += $(GL_RENDERER_CFLAGS)
libpepper_la_LIBADD += $(GL_RENDERER_LIBS)

libpepper_la_SOURCES += gl-renderer.c
endif

# wayland backend
if ENABLE_WAYLAND_BACKEND
include_HEADERS += modules/wayland/pepper-wayland.h

libpepper_la_CFLAGS += $(WAYLAND_BACKEND_CFLAGS) -I$(top_srcdir)/shared/
libpepper_la_LIBADD += $(WAYLAND_BACKEND_LIBS) $(top_builddir)/shared/libshared.la

libpepper_la_SOURCES += modules/wayland/wayland-internal.h  \
                        modules/wayland/wayland-common.c    \
                        modules/wayland/wayland-output.c    \
                        modules/wayland/wayland-input.c     \
                        modules/wayland/wayland-shm-buffer.c
endif

# drm backend
if ENABLE_DRM_BACKEND
include_HEADERS += modules/drm/pepper-drm.h

libpepper_la_CFLAGS += $(DRM_BACKEND_CFLAGS) -I$(top_srcdir)/src/modules/libinput/
libpepper_la_LIBADD += $(DRM_BACKEND_LIBS)

libpepper_la_SOURCES += modules/drm/drm-internal.h  \
                        modules/drm/drm-common.c
endif

# libinput module for drm & fbdev backends
if ENABLE_LIBINPUT
include_HEADERS += modules/libinput/pepper-libinput.h

libpepper_la_CFLAGS += $(LIBINPUT_CFLAGS)
libpepper_la_LIBADD += $(LIBINPUT_LIBS)

libpepper_la_SOURCES += modules/libinput/libinput-internal.h  \
                        modules/libinput/libinput.c
endif

# x11 backend
if ENABLE_X11_BACKEND
include_HEADERS += modules/x11/pepper-x11.h

libpepper_la_CFLAGS += $(X11_BACKEND_CFLAGS)
libpepper_la_LIBADD += $(X11_BACKEND_LIBS)

libpepper_la_SOURCES += modules/x11/x11-common.c    \
                        modules/x11/x11-output.c    \
                        modules/x11/x11-input.c     \
                        modules/x11/x11-internal.h
endif


# Pepper server executable
if ENABLE_PEPPER_SERVER
bin_PROGRAMS += pepper

pepper_LDFLAGS = -export-dynamic
pepper_CFLAGS = $(PEPPER_SERVER_CFLAGS) -I$(top_srcdir)/src
pepper_LDADD = $(PEPPER_SERVER_LIBS) -ldl libpepper.la

pepper_SOURCES = server/server.c
endif
