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

# Output files
AC_CONFIG_FILES([
Makefile
pepper.pc
])

AC_OUTPUT
