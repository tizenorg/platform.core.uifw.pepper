noinst_PROGRAMS =

# drm-backend
noinst_PROGRAMS += drm-backend

drm_backend_CFLAGS = $(WAYLAND_SERVER_CFLAGS)                   \
                     $(PEPPER_INTERNAL_CFLAGS)                  \
                     $(PEPPER_DRM_INTERNAL_CFLAGS)              \
                     $(PEPPER_DESKTOP_SHELL_INTERNAL_CFLAGS)

drm_backend_LDADD = $(WAYLAND_SERVER_LIBS)                      \
                    $(PEPPER_INTERNAL_LIBS)                     \
                    $(PEPPER_DRM_INTERNAL_LIBS)                 \
                    $(PEPPER_DESKTOP_SHELL_INTERNAL_LIBS)

drm_backend_SOURCES = drm-backend.c

# fbdev-backend
noinst_PROGRAMS += fbdev-backend

fbdev_backend_CFLAGS = $(WAYLAND_SERVER_CFLAGS)                 \
                       $(PEPPER_INTERNAL_CFLAGS)                \
                       $(PEPPER_FBDEV_INTERNAL_CFLAGS)          \
                       $(PEPPER_DESKTOP_SHELL_INTERNAL_CFLAGS)

fbdev_backend_LDADD = $(WAYLAND_SERVER_LIBS)                    \
                      $(PEPPER_INTERNAL_LIBS)                   \
                      $(PEPPER_FBDEV_INTERNAL_LIBS)             \
                      $(PEPPER_DESKTOP_SHELL_INTERNAL_LIBS)

fbdev_backend_SOURCES = fbdev-backend.c

# wayland-backend
noinst_PROGRAMS += wayland-backend

wayland_backend_CFLAGS = $(WAYLAND_SERVER_CFLAGS)               \
                         $(PEPPER_INTERNAL_CFLAGS)              \
                         $(PEPPER_WAYLAND_INTERNAL_CFLAGS)      \
                         $(PEPPER_DESKTOP_SHELL_INTERNAL_CFLAGS)

wayland_backend_LDADD = $(WAYLAND_SERVER_LIBS)                  \
                        $(PEPPER_INTERNAL_LIBS)                 \
                        $(PEPPER_WAYLAND_INTERNAL_LIBS)         \
                        $(PEPPER_DESKTOP_SHELL_INTERNAL_LIBS)

wayland_backend_SOURCES = wayland-backend.c

# x11-backend
if ENABLE_X11
noinst_PROGRAMS += x11-backend

x11_backend_CFLAGS = $(WAYLAND_SERVER_CFLAGS)                   \
                     $(PEPPER_INTERNAL_CFLAGS)                  \
                     $(PEPPER_X11_INTERNAL_CFLAGS)              \
                     $(PEPPER_DESKTOP_SHELL_INTERNAL_CFLAGS)

x11_backend_LDADD = $(WAYLAND_SERVER_LIBS)                      \
                    $(PEPPER_INTERNAL_LIBS)                     \
                    $(PEPPER_X11_INTERNAL_LIBS)                 \
                    $(PEPPER_DESKTOP_SHELL_INTERNAL_LIBS)

x11_backend_SOURCES = x11-backend.c
endif

# simple-touch
noinst_PROGRAMS += simple-touch

simple_touch_CFLAGS = $(WAYLAND_CLIENT_CFLAGS)  \
                      $(PEPPER_INTERNAL_CFLAGS)

simple_touch_LDADD = $(WAYLAND_CLIENT_LIBS)     \
                     $(PEPPER_INTERNAL_LIBS)

simple_touch_SOURCES = simple-touch.c

# simple-shm
noinst_PROGRAMS += simple-shm

simple_shm_CFLAGS = $(WAYLAND_CLIENT_CFLAGS)    \
                    $(PEPPER_INTERNAL_CFLAGS)

simple_shm_LDADD = $(WAYLAND_CLIENT_LIBS)       \
                   $(PEPPER_INTERNAL_LIBS)

simple_shm_SOURCES = simple-shm.c
