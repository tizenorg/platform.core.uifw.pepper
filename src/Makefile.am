SUBDIRS = lib/pepper        \
          lib/libinput      \
          lib/desktop-shell \
          lib/render        \
          lib/drm           \
          lib/fbdev         \
          lib/wayland

if ENABLE_TDM
SUBDIRS += lib/tdm
endif

if ENABLE_X11
SUBDIRS += lib/x11
endif

SUBDIRS += samples          \
           bin/doctor
