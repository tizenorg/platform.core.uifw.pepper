m4_define([pepper_x11_major], 0)
m4_define([pepper_x11_minor], 0)
m4_define([pepper_x11_micro], 0)

m4_define([pepper_x11_version], [pepper_x11_major.pepper_x11_minor.pepper_x11_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_x11], [pepper_x11_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_X11_VERSION_MAJOR], [pepper_x11_major_version])
AC_SUBST([PEPPER_X11_VERSION_MINOR], [pepper_x11_minor_version])
AC_SUBST([PEPPER_X11_VERSION_MICRO], [pepper_x11_micro_version])
AC_SUBST([PEPPER_X11_VERSION], [pepper_x11_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_X11_MODULES="pepper pepper-render x11 xcb-shm x11-xcb wayland-server pixman-1"
PKG_CHECK_MODULES(PEPPER_X11, [$PEPPER_X11_MODULES])
AC_SUBST([PEPPER_X11_REQUIRES], [$PEPPER_X11_MODULES])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-x11.pc
])

AC_OUTPUT
