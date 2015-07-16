m4_define([pepper_major], 0)
m4_define([pepper_minor], 0)
m4_define([pepper_micro], 0)

m4_define([pepper_version], [pepper_major.pepper_minor.pepper_micro])

AC_PREREQ([2.64])
AC_INIT([pepper], [pepper_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_VERSION_MAJOR], [pepper_major_version])
AC_SUBST([PEPPER_VERSION_MINOR], [pepper_minor_version])
AC_SUBST([PEPPER_VERSION_MICRO], [pepper_micro_version])
AC_SUBST([PEPPER_VERSION], [pepper_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_MODULES="wayland-server pixman-1"
PKG_CHECK_MODULES(PEPPER, [$PEPPER_MODULES])
AC_SUBST([PEPPER_REQUIRES], [$PEPPER_MODULES])

PEPPER_LIBINPUT_MODULES="wayland-server libinput pixman-1"
PKG_CHECK_MODULES(PEPPER_LIBINPUT, [$PEPPER_LIBINPUT_MODULES])
AC_SUBST([PEPPER_LIBINPUT_REQUIRES], [$PEPPER_LIBINPUT_MODULES])

PEPPER_DESKTOP_SHELL_MODULES="pixman-1"
PKG_CHECK_MODULES(PEPPER_DESKTOP_SHELL, [$PEPPER_DESKTOP_SHELL_MODULES])
AC_SUBST([PEPPER_DESKTOP_SHELL_REQUIRES], [$PEPPER_DESKTOP_SHELL_MODULES])

# wayland-scanner
AC_PATH_PROG([wayland_scanner], [wayland-scanner])
if test x$wayland_scanner = x; then
    PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
    wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
fi

PEPPER_RENDER_MODULES=""

# pixman renderer
AC_ARG_ENABLE(pixman,
              AC_HELP_STRING([--enable-pixman], [enable pixman renderer]),
              [enable_pixman=$enableval], [enable_pixman=yes])

AM_CONDITIONAL(ENABLE_PIXMAN, test $enable_pixman = yes)

if test $enable_pixman = yes; then
    AC_DEFINE(ENABLE_PIXMAN, 1, [Enable pixman renderer])
    PEPPER_RENDER_MODULES="$PEPPER_RENDER_MODULES pixman-1"
fi

# gl renderer
AC_ARG_ENABLE(gl,
              AC_HELP_STRING([--enable-gl], [enable OpenGL renderer]),
              [enable_gl=$enableval], [enable_gl=yes])

AM_CONDITIONAL(ENABLE_GL, test $enable_gl = yes)

if test $enable_gl = yes; then
    AC_DEFINE(ENABLE_GL, 1, [Enable OpenGL renderer])
    PEPPER_RENDER_MODULES="$PEPPER_RENDER_MODULES egl glesv2"
fi

PKG_CHECK_MODULES(PEPPER_RENDER, [$PEPPER_RENDER_MODULES])
AC_SUBST([PEPPER_RENDER_REQUIRES], [$PEPPER_RENDER_MODULES])

PEPPER_DRM_MODULES="pixman-1 wayland-server libdrm gbm"
PKG_CHECK_MODULES(PEPPER_DRM, [$PEPPER_DRM_MODULES])
AC_SUBST([PEPPER_DRM_REQUIRES], [$PEPPER_DRM_MODULES])

PEPPER_X11_MODULES="x11 xcb-shm x11-xcb wayland-server pixman-1"
PKG_CHECK_MODULES(PEPPER_X11, [$PEPPER_X11_MODULES])
AC_SUBST([PEPPER_X11_REQUIRES], [$PEPPER_X11_MODULES])

PEPPER_FBDEV_MODULES="pixman-1 wayland-server"
PKG_CHECK_MODULES(PEPPER_FBDEV, [$PEPPER_FBDEV_MODULES])
AC_SUBST([PEPPER_FBDEV_REQUIRES], [$PEPPER_FBDEV_MODULES])

PEPPER_WAYLAND_MODULES="wayland-server wayland-client pixman-1"
PKG_CHECK_MODULES(PEPPER_WAYLAND, [$PEPPER_WAYLAND_MODULES])
AC_SUBST([PEPPER_WAYLAND_REQUIRES], [$PEPPER_WAYLAND_MODULES])

DOCTOR_MODULES="wayland-server pixman-1"
PKG_CHECK_MODULES(DOCTOR, [$DOCTOR_MODULES])

DRM_BACKEND_MODULES="pixman-1 wayland-server"
PKG_CHECK_MODULES(DRM_BACKEND, [$DRM_BACKEND_MODULES])

FBDEV_BACKEND_MODULES="pixman-1 wayland-server"
PKG_CHECK_MODULES(FBDEV_BACKEND, [$FBDEV_BACKEND_MODULES])

WAYLAND_BACKEND_MODULES="pixman-1 wayland-server"
PKG_CHECK_MODULES(WAYLAND_BACKEND, [$WAYLAND_BACKEND_MODULES])

X11_BACKEND_MODULES="pixman-1 wayland-server"
PKG_CHECK_MODULES(X11_BACKEND, [$X11_BACKEND_MODULES])

SIMPLE_TOUCH_MODULES="wayland-client"
PKG_CHECK_MODULES(SIMPLE_TOUCH, [$SIMPLE_TOUCH_MODULES])

SIMPLE_SHM_MODULES="pixman-1 wayland-client"
PKG_CHECK_MODULES(SIMPLE_SHM, [$SIMPLE_SHM_MODULES])

# Output files
AC_CONFIG_FILES([
Makefile
src/Makefile
src/lib/Makefile
src/lib/pepper/Makefile
src/lib/libinput/Makefile
src/lib/desktop-shell/Makefile
src/lib/render/Makefile
src/lib/drm/Makefile
src/lib/x11/Makefile
src/lib/fbdev/Makefile
src/lib/wayland/Makefile
src/bin/Makefile
src/bin/doctor/Makefile
src/samples/Makefile
])

AC_OUTPUT
