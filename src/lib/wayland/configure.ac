m4_define([pepper_wayland_major], 0)
m4_define([pepper_wayland_minor], 0)
m4_define([pepper_wayland_micro], 0)

m4_define([pepper_wayland_version], [pepper_wayland_major.pepper_wayland_minor.pepper_wayland_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_wayland], [pepper_wayland_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_WAYLAND_VERSION_MAJOR], [pepper_wayland_major_version])
AC_SUBST([PEPPER_WAYLAND_VERSION_MINOR], [pepper_wayland_minor_version])
AC_SUBST([PEPPER_WAYLAND_VERSION_MICRO], [pepper_wayland_micro_version])
AC_SUBST([PEPPER_WAYLAND_VERSION], [pepper_wayland_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_WAYLAND_MODULES="pepper pepper-render wayland-server wayland-client pixman-1"
PKG_CHECK_MODULES(PEPPER_WAYLAND, [$PEPPER_WAYLAND_MODULES])
AC_SUBST([PEPPER_WAYLAND_REQUIRES], [$PEPPER_WAYLAND_MODULES])

AC_CONFIG_FILES([
Makefile
pepper-wayland.pc
])

AC_OUTPUT
