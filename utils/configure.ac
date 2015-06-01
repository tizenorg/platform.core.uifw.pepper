m4_define([pepper_utils_major], 0)
m4_define([pepper_utils_minor], 0)
m4_define([pepper_utils_micro], 0)

m4_define([pepper_utils_version], [pepper_utils_major.pepper_utils_minor.pepper_utils_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_utils], [pepper_utils_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_UTILS_VERSION_MAJOR], [pepper_utils_major_version])
AC_SUBST([PEPPER_UTILS_VERSION_MINOR], [pepper_utils_minor_version])
AC_SUBST([PEPPER_UTILS_VERSION_MICRO], [pepper_utils_micro_version])
AC_SUBST([PEPPER_UTILS_VERSION], [pepper_utils_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_CHECK_MODULES(PEPPER_UTILS, [pepper])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-utils.pc
])

AC_OUTPUT
