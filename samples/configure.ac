m4_define([pepper_samples_major], 0)
m4_define([pepper_samples_minor], 0)
m4_define([pepper_samples_micro], 0)

m4_define([pepper_samples_version], [pepper_samples_major.pepper_samples_minor.pepper_samples_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_samples], [pepper_samples_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_SAMPLES_VERSION_MAJOR], [pepper_samples_major_version])
AC_SUBST([PEPPER_SAMPLES_VERSION_MINOR], [pepper_samples_minor_version])
AC_SUBST([PEPPER_SAMPLES_VERSION_MICRO], [pepper_samples_micro_version])
AC_SUBST([PEPPER_SAMPLES_VERSION], [pepper_samples_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

# drm-backend
PKG_CHECK_MODULES(DRM_BACKEND,
                  [wayland-server pepper pepper-render
                   pepper-drm pepper-libinput
                   pepper-desktop-shell])

# wayland-backend
PKG_CHECK_MODULES(WAYLAND_BACKEND,
                  [wayland-server pepper pepper-render pepper-wayland pepper-desktop-shell])

# x11-backend
PKG_CHECK_MODULES(X11_BACKEND,
                  [wayland-server pepper pepper-render pepper-x11 pepper-desktop-shell])

# simple-touch
PKG_CHECK_MODULES(SIMPLE_TOUCH, [wayland-client pepper])

# simple-shm
PKG_CHECK_MODULES(SIMPLE_SHM, [wayland-client pepper])

AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT
