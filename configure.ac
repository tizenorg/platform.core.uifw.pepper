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

# pepper
PEPPER_REQUIRES="wayland-server pixman-1"
PKG_CHECK_MODULES(PEPPER, [$PEPPER_REQUIRES])

PEPPER_INTERNAL_CFLAGS="$PEPPER_CFLAGS -I\$(top_srcdir)/src/lib/pepper"
PEPPER_INTERNAL_LIBS="\$(top_srcdir)/src/lib/pepper/libpepper.la"

AC_SUBST(PEPPER_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_INTERNAL_LIBS)
AC_SUBST(PEPPER_REQUIRES)

# pepper-libinput
PEPPER_LIBINPUT_REQUIRES="libinput"
PKG_CHECK_MODULES(PEPPER_LIBINPUT, [$PEPPER_LIBINPUT_REQUIRES])
PEPPER_LIBINPUT_REQUIRES="$PEPPER_LIBINPUT_REQUIRES pepper"

PEPPER_LIBINPUT_INTERNAL_CFLAGS="$PEPPER_INTERNAL_CFLAGS -I\$(top_srcdir)/src/lib/libinput"
PEPPER_LIBINPUT_INTERNAL_LIBS="\$(top_srcdir)/src/lib/libinput/libpepper-libinput.la"

AC_SUBST(PEPPER_LIBINPUT_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_LIBINPUT_INTERNAL_LIBS)
AC_SUBST(PEPPER_LIBINPUT_REQUIRES)

# pepper-desktop-shell
PEPPER_DESKTOP_SHELL_REQUIRES="pepper"

PEPPER_DESKTOP_SHELL_INTERNAL_CFLAGS="$PEPPER_INTERNAL_CFLAGS -I\$(top_srcdir)/src/lib/desktop-shell"
PEPPER_DESKTOP_SHELL_INTERNAL_LIBS="\$(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la"

AC_SUBST(PEPPER_DESKTOP_SHELL_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_DESKTOP_SHELL_INTERNAL_LIBS)
AC_SUBST(PEPPER_DESKTOP_SHELL_REQUIRES)

# pepper-render
PEPPER_RENDER_REQUIRES="pixman-1 egl glesv2"
PKG_CHECK_MODULES(PEPPER_RENDER, [$PEPPER_RENDER_REQUIRES])
PEPPER_RENDER_REQUIRES="$PEPPER_RENDER_REQUIRES pepper"

PEPPER_RENDER_INTERNAL_CFLAGS="$PEPPER_INTERNAL_CFLAGS -I\$(top_srcdir)/src/lib/render"
PEPPER_RENDER_INTERNAL_LIBS="\$(top_srcdir)/src/lib/render/libpepper-render.la"

AC_SUBST(PEPPER_RENDER_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_RENDER_INTERNAL_LIBS)
AC_SUBST(PEPPER_RENDER_REQUIRES)

# pepper-drm
PEPPER_DRM_REQUIRES="libdrm gbm"
PKG_CHECK_MODULES(PEPPER_DRM, [$PEPPER_DRM_REQUIRES])
PEPPER_DRM_REQUIRES="$PEPPER_DRM_REQUIRES pepper pepper-render"

PEPPER_DRM_INTERNAL_CFLAGS="$PEPPER_CFLAGS $PEPPER_RENDER_CFLAGS -I\$(top_srcdir)/src/lib/drm"
PEPPER_DRM_INTERNAL_LIBS="\$(top_srcdir)/src/lib/drm/libpepper-drm.la"

AC_SUBST(PEPPER_DRM_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_DRM_INTERNAL_LIBS)
AC_SUBST(PEPPER_DRM_REQUIRES)

# pepper-fbdev
PEPPER_FBDEV_REQUIRES="pepper pepper-render"

PEPPER_FBDEV_INTERNAL_CFLAGS="$PEPPER_CFLAGS $PEPPER_RENDER_CFLAGS -I\$(top_srcdir)/src/lib/fbdev"
PEPPER_FBDEV_INTERNAL_LIBS="\$(top_srcdir)/src/lib/fbdev/libpepper-fbdev.la"

AC_SUBST(PEPPER_FBDEV_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_FBDEV_INTERNAL_LIBS)
AC_SUBST(PEPPER_FBDEV_REQUIRES)

# pepper-wayland
PEPPER_WAYLAND_REQUIRES="wayland-client"
PKG_CHECK_MODULES(PEPPER_WAYLAND, [$PEPPER_WAYLAND_REQUIRES])
PEPPER_WAYLAND_REQUIRES="$PEPPER_WAYLAND_REQUIRES pepper pepper-render"

PEPPER_WAYLAND_INTERNAL_CFLAGS="$PEPPER_CFLAGS $PEPPER_RENDER_CFLAGS -I\$(top_srcdir)/src/lib/wayland"
PEPPER_WAYLAND_INTERNAL_LIBS="\$(top_srcdir)/src/lib/wayland/libpepper-wayland.la"

AC_SUBST(PEPPER_WAYLAND_INTERNAL_CFLAGS)
AC_SUBST(PEPPER_WAYLAND_INTERNAL_LIBS)
AC_SUBST(PEPPER_WAYLAND_REQUIRES)

# pepper-x11
AC_ARG_ENABLE(x11,
              AC_HELP_STRING([--disable-x11],
                             [disable x11 backend module]),
              enable_x11=$enableval,
              enable_x11=yes)
AM_CONDITIONAL(ENABLE_X11, test x$enable_x11 = xyes)
if test x$enable_x11 = xyes; then
    PEPPER_X11_REQUIRES="x11 xcb-shm x11-xcb"
    PKG_CHECK_MODULES(PEPPER_X11, [$PEPPER_X11_REQUIRES])
    PEPPER_X11_REQUIRES="$PEPPER_X11_REQUIRES pepper pepper-render"

    PEPPER_X11_INTERNAL_CFLAGS="$PEPPER_CFLAGS $PEPPER_RENDER_CFLAGS -I\$(top_srcdir)/src/lib/x11"
    PEPPER_X11_INTERNAL_LIBS="\$(top_srcdir)/src/lib/x11/libpepper-x11.la"

    AC_SUBST(PEPPER_X11_INTERNAL_CFLAGS)
    AC_SUBST(PEPPER_X11_INTERNAL_LIBS)
    AC_SUBST(PEPPER_X11_REQUIRES)
fi

# doctor
DOCTOR_REQUIRES="wayland-server"
PKG_CHECK_MODULES(DOCTOR, [$DOCTOR_REQUIRES])
DOCTOR_REQUIRES="$DOCTOR_REQUIRES pepper pepper-drm pepper-desktop-shell"

DOCTOR_CFLAGS="$DOCTOR_CFLAGS $PEPPER_CFLAGS $PEPPER_DRM_CFLAGS $PEPPER_DEKSTOP_SHELL_CFLAGS"
DOCTOR_LIBS="$DOCTOR_CFLAGS $PEPPER_LIBS $PEPPER_DRM_LIBS $PEPPER_DESKTOP_SHELL_LIBS"

AC_SUBST(DOCTOR_CFLAGS)
AC_SUBST(DOCTOR_LIBS)

# sample
PKG_CHECK_MODULES(WAYLAND_SERVER, ["wayland-server"])
PKG_CHECK_MODULES(WAYLAND_CLIENT, ["wayland-client"])

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
pkgconfig/pepper-desktop-shell.pc
])

if test x$enable_x11 = xyes; then
    AC_CONFIG_FILES([
        src/lib/x11/Makefile
        pkgconfig/pepper-x11.pc
    ])
fi

AC_OUTPUT
