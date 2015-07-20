noinst_PROGRAMS =

# drm-backend
noinst_PROGRAMS += drm-backend

drm_backend_CFLAGS = -I$(top_srcdir)/src/lib/pepper         \
                     -I$(top_srcdir)/src/lib/drm            \
                     -I$(top_srcdir)/src/lib/desktop-shell  \
                     $(PIXMAN_CFLAGS)                       \
                     $(WAYLAND_SERVER_CFLAGS)

drm_backend_LDADD = $(top_srcdir)/src/lib/pepper/libpepper.la                       \
                    $(top_srcdir)/src/lib/drm/libpepper-drm.la                      \
                    $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la  \
                    $(WAYLAND_SERVER_LIBS)

drm_backend_SOURCES = drm-backend.c

# fbdev-backend
noinst_PROGRAMS += fbdev-backend

fbdev_backend_CFLAGS = -I$(top_srcdir)/src/lib/pepper           \
                       -I$(top_srcdir)/src/lib/fbdev            \
                       -I$(top_srcdir)/src/lib/desktop-shell    \
                       $(PIXMAN_CFLAGS)                         \
                       $(WAYLAND_SERVER_CFLAGS)

fbdev_backend_LDADD = $(top_srcdir)/src/lib/pepper/libpepper.la                         \
                      $(top_srcdir)/src/lib/fbdev/libpepper-fbdev.la                    \
                      $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la    \
                      $(WAYLAND_SERVER_LIBS)

fbdev_backend_SOURCES = fbdev-backend.c

# wayland-backend
noinst_PROGRAMS += wayland-backend

wayland_backend_CFLAGS = -I$(top_srcdir)/src/lib/pepper         \
                         -I$(top_srcdir)/src/lib/wayland        \
                         -I$(top_srcdir)/src/lib/desktop-shell  \
                         $(PIXMAN_CFLAGS)                       \
                         $(WAYLAND_SERVER_CFLAGS)

wayland_backend_LDADD = $(top_srcdir)/src/lib/pepper/libpepper.la                       \
                        $(top_srcdir)/src/lib/wayland/libpepper-wayland.la              \
                        $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la  \
                        $(WAYLAND_SERVER_LIBS)

wayland_backend_SOURCES = wayland-backend.c

# x11-backend
noinst_PROGRAMS += x11-backend

x11_backend_CFLAGS = -I$(top_srcdir)/src/lib/pepper         \
                     -I$(top_srcdir)/src/lib/x11            \
                     -I$(top_srcdir)/src/lib/desktop-shell  \
                     $(PIXMAN_CFLAGS)                       \
                     $(WAYLAND_SERVER_CFLAGS)

x11_backend_LDADD = $(top_srcdir)/src/lib/pepper/libpepper.la                       \
                    $(top_srcdir)/src/lib/x11/libpepper-x11.la                      \
                    $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la  \
                    $(WAYLAND_SERVER_LIBS)

x11_backend_SOURCES = x11-backend.c

# simple-touch
noinst_PROGRAMS += simple-touch

simple_touch_CFLAGS = $(WAYLAND_CLIENT_CFLAGS)      \
                      -I$(top_srcdir)/src/lib/pepper  \
                      $(PIXMAN_CFLAGS)

simple_touch_LDADD = $(WAYLAND_CLIENT_LIBS)                     \
                     $(top_srcdir)/src/lib/pepper/libpepper.la

simple_touch_SOURCES = simple-touch.c

# simple-shm
noinst_PROGRAMS += simple-shm

simple_shm_CFLAGS = $(WAYLAND_CLIENT_CFLAGS)        \
                    -I$(top_srcdir)/src/lib/pepper    \
                    $(PIXMAN_CFLAGS)

simple_shm_LDADD = $(WAYLAND_CLIENT_LIBS)                       \
                     $(top_srcdir)/src/lib/pepper/libpepper.la

simple_shm_SOURCES = simple-shm.c
