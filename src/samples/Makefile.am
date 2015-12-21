noinst_PROGRAMS =

# drm-backend
noinst_PROGRAMS += drm-backend

drm_backend_CFLAGS = $(SAMPLES_CFLAGS)
drm_backend_LDADD  = $(SAMPLES_LIBS)

drm_backend_SOURCES = drm-backend.c

# tdm-backend
if ENABLE_TDM
noinst_PROGRAMS += tdm-backend

tdm_backend_CFLAGS = $(SAMPLES_CFLAGS)
tdm_backend_LDADD  = $(SAMPLES_LIBS)

tdm_backend_SOURCES = tdm-backend.c
endif

# fbdev-backend
noinst_PROGRAMS += fbdev-backend

fbdev_backend_CFLAGS = $(SAMPLES_CFLAGS)
fbdev_backend_LDADD  = $(SAMPLES_LIBS)

fbdev_backend_SOURCES = fbdev-backend.c

# wayland-backend
noinst_PROGRAMS += wayland-backend

wayland_backend_CFLAGS = $(SAMPLES_CFLAGS)
wayland_backend_LDADD  = $(SAMPLES_LIBS)

wayland_backend_SOURCES = wayland-backend.c

# x11-backend
if ENABLE_X11
noinst_PROGRAMS += x11-backend

x11_backend_CFLAGS = $(SAMPLES_CFLAGS)
x11_backend_LDADD  = $(SAMPLES_LIBS)

x11_backend_SOURCES = x11-backend.c
endif

