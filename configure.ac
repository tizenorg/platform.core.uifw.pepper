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

PKG_CHECK_MODULES(WAYLAND_SERVER, "wayland-server")
PKG_CHECK_MODULES(WAYLAND_CLIENT, "wayland-client")
PKG_CHECK_MODULES(PIXMAN, "pixman-1")
PKG_CHECK_MODULES(LIBINPUT, "libinput")
PKG_CHECK_MODULES(LIBDRM, "libdrm")
PKG_CHECK_MODULES(GBM, "gbm")
PKG_CHECK_MODULES(X11, "x11 xcb-shm x11-xcb")
PKG_CHECK_MODULES(EGL, "egl")
PKG_CHECK_MODULES(GLESV2, "glesv2")

# wayland-scanner
AC_PATH_PROG([wayland_scanner], [wayland-scanner])
if test x$wayland_scanner = x; then
    PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
    wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
fi

# Output files
AC_CONFIG_FILES([
Makefile
src/Makefile
src/lib/pepper/Makefile
src/lib/libinput/Makefile
src/lib/desktop-shell/Makefile
src/lib/render/Makefile
src/lib/drm/Makefile
src/lib/x11/Makefile
src/lib/fbdev/Makefile
src/lib/wayland/Makefile
src/bin/doctor/Makefile
src/samples/Makefile
pkgconfig/pepper.pc
pkgconfig/pepper-render.pc
pkgconfig/pepper-drm.pc
pkgconfig/pepper-fbdev.pc
pkgconfig/pepper-libinput.pc
pkgconfig/pepper-wayland.pc
pkgconfig/pepper-x11.pc
pkgconfig/pepper-desktop-shell.pc
])

AC_OUTPUT
