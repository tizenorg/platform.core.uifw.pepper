noinst_PROGRAMS =

# drm-backend
noinst_PROGRAMS += drm-backend
drm_backend_CFLAGS = $(DRM_BACKEND_CFLAGS)          \
                     -I$(top_srcdir)/src/lib/pepper \
                     -I$(top_srcdir)/src/lib/drm    \
                     -I$(top_srcdir)/src/lib/desktop-shell
drm_backend_LDADD = $(DRM_BACKEND_LIBS)                        \
                    $(top_srcdir)/src/lib/pepper/libpepper.la  \
                    $(top_srcdir)/src/lib/drm/libpepper-drm.la \
                    $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la
drm_backend_SOURCES = drm-backend.c

# fbdev-backend
noinst_PROGRAMS += fbdev-backend
fbdev_backend_CFLAGS = $(FBDEV_BACKEND_CFLAGS)        \
                       -I$(top_srcdir)/src/lib/pepper \
                       -I$(top_srcdir)/src/lib/fbdev  \
                       -I$(top_srcdir)/src/lib/desktop-shell
fbdev_backend_LDADD = $(FBDEV_BACKEND_LIBS)                          \
                      $(top_srcdir)/src/lib/pepper/libpepper.la      \
                      $(top_srcdir)/src/lib/fbdev/libpepper-fbdev.la \
                      $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la
fbdev_backend_SOURCES = fbdev-backend.c

# wayland-backend
noinst_PROGRAMS += wayland-backend
wayland_backend_CFLAGS = $(WAYLAND_BACKEND_CFLAGS)       \
                         -I$(top_srcdir)/src/lib/pepper  \
                         -I$(top_srcdir)/src/lib/wayland \
                         -I$(top_srcdir)/src/lib/desktop-shell
wayland_backend_LDADD = $(WAYLAND_BACKEND_LIBS)                            \
                        $(top_srcdir)/src/lib/pepper/libpepper.la          \
                        $(top_srcdir)/src/lib/wayland/libpepper-wayland.la \
                        $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la
wayland_backend_SOURCES = wayland-backend.c

# x11-backend
noinst_PROGRAMS += x11-backend
x11_backend_CFLAGS = $(X11_BACKEND_CFLAGS)          \
                     -I$(top_srcdir)/src/lib/pepper \
                     -I$(top_srcdir)/src/lib/x11    \
                     -I$(top_srcdir)/src/lib/desktop-shell
x11_backend_LDADD = $(X11_BACKEND_LIBS)                        \
                    $(top_srcdir)/src/lib/pepper/libpepper.la  \
                    $(top_srcdir)/src/lib/x11/libpepper-x11.la \
                    $(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la
x11_backend_SOURCES = x11-backend.c

# simple-touch
noinst_PROGRAMS += simple-touch
simple_touch_CFLAGS = $(SIMPLE_TOUCH_CFLAGS)
simple_touch_LDADD = $(SIMPLE_TOUCH_LIBS) \
                     $(top_srcdir)/src/lib/pepper/libpepper.la
simple_touch_SOURCES = simple-touch.c

# simple-shm
noinst_PROGRAMS += simple-shm
simple_shm_CFLAGS = $(SIMPLE_SHM_CFLAGS) \
                    -I$(top_srcdir)/src/lib/pepper
simple_shm_LDADD = $(SIMPLE_SHM_LIBS) \
                   $(top_srcdir)/src/lib/pepper/libpepper.la
simple_shm_SOURCES = simple-shm.c
