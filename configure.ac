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

if test "x$GCC" = "xyes"; then
GCC_CFLAGS="-Wall -Wextra -Wno-unused-parameter \
            -Wno-missing-field-initializers -g -fvisibility=hidden \
            -Wstrict-prototypes -Wmissing-prototypes -Wsign-compare"
fi
AC_SUBST(GCC_CFLAGS)

# pepper
PEPPER_REQUIRES="wayland-server pixman-1 xkbcommon"
PKG_CHECK_MODULES(PEPPER, [$PEPPER_REQUIRES])

PEPPER_DIR="-I\$(top_srcdir)/src/lib/pepper"
PEPPER_LIB="\$(top_srcdir)/src/lib/pepper/libpepper.la"

AC_SUBST(PEPPER_REQUIRES)

# pepper-libinput
PEPPER_LIBINPUT_REQUIRES="libinput"
PKG_CHECK_MODULES(PEPPER_LIBINPUT, [$PEPPER_LIBINPUT_REQUIRES])
PEPPER_LIBINPUT_REQUIRES="$PEPPER_LIBINPUT_REQUIRES pepper"

PEPPER_LIBINPUT_DIR="-I\$(top_srcdir)/src/lib/libinput"
PEPPER_LIBINPUT_LIB="\$(top_srcdir)/src/lib/libinput/libpepper-libinput.la"

PEPPER_LIBINPUT_CFLAGS="$PEPPER_DIR $PEPPER_LIBINPUT_CFLAGS $PEPPER_CFLAGS"
PEPPER_LIBINPUT_LIBS="$PEPPER_LIB $PEPPER_LIBINPUT_LIBS"

AC_SUBST(PEPPER_LIBINPUT_CFLAGS)
AC_SUBST(PEPPER_LIBINPUT_LIBS)
AC_SUBST(PEPPER_LIBINPUT_REQUIRES)

# pepper-desktop-shell
PEPPER_DESKTOP_SHELL_REQUIRES="pepper"

PEPPER_DESKTOP_SHELL_DIR="-I\$(top_srcdir)/src/lib/desktop-shell"
PEPPER_DESKTOP_SHELL_LIB="\$(top_srcdir)/src/lib/desktop-shell/libpepper-desktop-shell.la"

PEPPER_DESKTOP_SHELL_CFLAGS="$PEPPER_DIR $PEPPER_CFLAGS"
PEPPER_DESKTOP_SHELL_LIBS="$PEPPER_LIB"

AC_SUBST(PEPPER_DESKTOP_SHELL_CFLAGS)
AC_SUBST(PEPPER_DESKTOP_SHELL_LIBS)
AC_SUBST(PEPPER_DESKTOP_SHELL_REQUIRES)

# pepper-render
PEPPER_RENDER_REQUIRES="pixman-1 egl glesv2"
PKG_CHECK_MODULES(PEPPER_RENDER, [$PEPPER_RENDER_REQUIRES])
PEPPER_RENDER_REQUIRES="$PEPPER_RENDER_REQUIRES pepper"

PEPPER_RENDER_DIR="-I\$(top_srcdir)/src/lib/render"
PEPPER_RENDER_LIB="\$(top_srcdir)/src/lib/render/libpepper-render.la"

PEPPER_RENDER_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_CFLAGS $PEPPER_CFLAGS"
PEPPER_RENDER_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIBS"

AC_SUBST(PEPPER_RENDER_CFLAGS)
AC_SUBST(PEPPER_RENDER_LIBS)
AC_SUBST(PEPPER_RENDER_REQUIRES)

# pepper-drm
PEPPER_DRM_REQUIRES="libdrm gbm"
PKG_CHECK_MODULES(PEPPER_DRM, [$PEPPER_DRM_REQUIRES])
PEPPER_DRM_REQUIRES="$PEPPER_DRM_REQUIRES pepper pepper-render pepper-libinput"

PKG_CHECK_MODULES([TBM], [libtbm wayland-tbm-server], [have_tbm=yes], [have_tbm=no])
if test x$have_tbm = xyes; then
   AC_DEFINE([HAVE_TBM], [1], [Build the tbm backend])
   PEPPER_DRM_REQUIRES="$PEPPER_DRM_REQUIRES libtbm wayland-tbm-server" 
fi

PEPPER_DRM_DIR="-I\$(top_srcdir)/src/lib/drm"
PEPPER_DRM_LIB="\$(top_srcdir)/src/lib/drm/libpepper-drm.la"

PEPPER_DRM_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_LIBINPUT_DIR $PEPPER_DRM_CFLAGS $TBM_CFLAGS"
PEPPER_DRM_CFLAGS="$PEPPER_DRM_CFLAGS $PEPPER_CFLAGS"
PEPPER_DRM_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_LIBINPUT_LIB $PEPPER_DRM_LIBS $TBM_LIBS"

AC_SUBST(PEPPER_DRM_CFLAGS)
AC_SUBST(PEPPER_DRM_LIBS)
AC_SUBST(PEPPER_DRM_REQUIRES)

# pepper-fbdev
PEPPER_FBDEV_REQUIRES="pepper pepper-render"

PEPPER_FBDEV_DIR="-I\$(top_srcdir)/src/lib/fbdev"
PEPPER_FBDEV_LIB="\$(top_srcdir)/src/lib/fbdev/libpepper-fbdev.la"

PEPPER_FBDEV_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_LIBINPUT_DIR $PEPPER_FBDEV_CFLAGS"
PEPPER_FBDEV_CFLAGS="$PEPPER_FBDEV_CFLAGS $PEPPER_CFLAGS"
PEPPER_FBDEV_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_LIBINPUT_LIB $PEPPER_FBDEV_LIBS"

AC_SUBST(PEPPER_FBDEV_CFLAGS)
AC_SUBST(PEPPER_FBDEV_LIBS)
AC_SUBST(PEPPER_FBDEV_REQUIRES)

# pepper-wayland
PEPPER_WAYLAND_REQUIRES="wayland-client"
PKG_CHECK_MODULES(PEPPER_WAYLAND, [$PEPPER_WAYLAND_REQUIRES])
PEPPER_WAYLAND_REQUIRES="$PEPPER_WAYLAND_REQUIRES pepper pepper-render"

PEPPER_WAYLAND_DIR="-I\$(top_srcdir)/src/lib/wayland"
PEPPER_WAYLAND_LIB="\$(top_srcdir)/src/lib/wayland/libpepper-wayland.la"

PEPPER_WAYLAND_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_WAYLAND_CFLAGS $PEPPER_CFLAGS"
PEPPER_WAYLAND_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_WAYLAND_LIBS"

AC_SUBST(PEPPER_WAYLAND_CFLAGS)
AC_SUBST(PEPPER_WAYLAND_LIBS)
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

    PEPPER_X11_DIR="-I\$(top_srcdir)/src/lib/x11"
    PEPPER_X11_LIB="\$(top_srcdir)/src/lib/x11/libpepper-x11.la"

    PEPPER_X11_CFLAGS="$PEPPER_DIR $PEPPER_RENDER_DIR $PEPPER_X11_CFLAGS $PEPPER_CFLAGS"
    PEPPER_X11_LIBS="$PEPPER_LIB $PEPPER_RENDER_LIB $PEPPER_X11_LIBS"

    AC_SUBST(PEPPER_X11_CFLAGS)
    AC_SUBST(PEPPER_X11_LIBS)
    AC_SUBST(PEPPER_X11_REQUIRES)
fi

# doctor
DOCTOR_REQUIRES="wayland-server pixman-1"
PKG_CHECK_MODULES(DOCTOR, [$DOCTOR_REQUIRES])
DOCTOR_REQUIRES="$DOCTOR_REQUIRES pepper pepper-drm pepper-desktop-shell"

DOCTOR_CFLAGS="$PEPPER_DIR $PEPPER_DRM_DIR $PEPPER_DESKTOP_SHELL_DIR $DOCTOR_CFLAGS"
DOCTOR_LIBS="$PEPPER_LIB $PEPPER_DRM_LIB $PEPPER_DESKTOP_SHELL_LIB $DOCTOR_LIBS"

AC_SUBST(DOCTOR_CFLAGS)
AC_SUBST(DOCTOR_LIBS)

# sample
SAMPLES_REQUIRES="wayland-server wayland-client libudev"
PKG_CHECK_MODULES(SAMPLES, $[SAMPLES_REQUIRES])

SAMPLES_CFLAGS="$PEPPER_DIR $PEPPER_DESKTOP_SHELL_DIR $PEPPER_RENDER_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$PEPPER_LIBINPUT_DIR $PEPPER_DRM_DIR $PEPPER_FBDEV_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$PEPPER_WAYLAND_DIR $PEPPER_X11_DIR $SAMPLES_CFLAGS"
SAMPLES_CFLAGS="$SAMPLES_CFLAGS $PEPPER_CFLAGS"

SAMPLES_LIBS="$PEPPER_LIB $PEPPER_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_DESKTOP_SHELL_LIB $PEPPER_DESKTOP_SHELL_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_RENDER_LIB $PEPPER_RENDER_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_LIBINPUT_LIB $PEPPER_LIBINPUT_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_DRM_LIB $PEPPER_DRM_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_FBDEV_LIB $PEPPER_FBDEV_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_WAYLAND_LIB $PEPPER_WAYLAND_LIBS $SAMPLES_LIBS"
SAMPLES_LIBS="$PEPPER_X11_LIB $PEPPER_X11_LIBS $SAMPLES_LIBS"

AC_SUBST(SAMPLES_CFLAGS)
AC_SUBST(SAMPLES_LIBS)

# wayland-scanner
AC_PATH_PROG([wayland_scanner], [wayland-scanner])
if test x$wayland_scanner = x; then
    PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
    wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
fi

# document
AC_ARG_ENABLE([document],
              [AC_HELP_STRING([--disable-document],
                              [Disable building the documentation])],
              [],
              [enable_document=yes])

AM_CONDITIONAL(ENABLE_DOCUMENT, test x$enable_document = xyes)

if test x$enable_document = xyes; then
    AC_PATH_PROG(DOXYGEN, [doxygen])
    if test x$DOXYGEN = x; then
        AC_MSG_ERROR([Documentation need doxygen, but not found])
    fi
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

if test x$enable_document = xyes; then
    AC_CONFIG_FILES([
        doc/Makefile
        doc/doxygen/pepper.doxygen
    ])
fi

AC_OUTPUT
