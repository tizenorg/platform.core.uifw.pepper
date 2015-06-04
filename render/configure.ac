m4_define([pepper_render_major], 0)
m4_define([pepper_render_minor], 0)
m4_define([pepper_render_micro], 0)

m4_define([pepper_render_version], [pepper_render_major.pepper_render_minor.pepper_render_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_render], [pepper_render_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_RENDER_VERSION_MAJOR], [pepper_render_major_version])
AC_SUBST([PEPPER_RENDER_VERSION_MINOR], [pepper_render_minor_version])
AC_SUBST([PEPPER_RENDER_VERSION_MICRO], [pepper_render_micro_version])
AC_SUBST([PEPPER_RENDER_VERSION], [pepper_render_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_RENDER_MODULES="pepper"

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

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-render.pc
])

AC_OUTPUT
