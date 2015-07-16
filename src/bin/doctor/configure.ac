m4_define([doctor_major], 0)
m4_define([doctor_minor], 0)
m4_define([doctor_micro], 0)

m4_define([doctor_version], [doctor_major.doctor_minor.doctor_micro])

AC_PREREQ([2.64])
AC_INIT([doctor], [doctor_version], [tkq.kim@samsung.com])

AC_SUBST([DOCTOR_VERSION_MAJOR], [doctor_major_version])
AC_SUBST([DOCTOR_VERSION_MINOR], [doctor_minor_version])
AC_SUBST([DOCTOR_VERSION_MICRO], [doctor_micro_version])
AC_SUBST([DOCTOR_VERSION], [doctor_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

DOCTOR_MODULES="wayland-server pepper"

PKG_CHECK_MODULES(DOCTOR, [$DOCTOR_MODULES])

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
