m4_define([pepper_libinput_major], 0)
m4_define([pepper_libinput_minor], 0)
m4_define([pepper_libinput_micro], 0)

m4_define([pepper_libinput_version],
          [pepper_libinput_major.pepper_libinput_minor.pepper_libinput_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_libinput], [pepper_libinput_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_LIBINPUT_VERSION_MAJOR], [pepper_libinput_major_version])
AC_SUBST([PEPPER_LIBINPUT_VERSION_MINOR], [pepper_libinput_minor_version])
AC_SUBST([PEPPER_LIBINPUT_VERSION_MICRO], [pepper_libinput_micro_version])
AC_SUBST([PEPPER_LIBINPUT_VERSION], [pepper_libinput_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_LIBINPUT_MODULES="pepper wayland-server libinput"
PKG_CHECK_MODULES(PEPPER_LIBINPUT, [$PEPPER_LIBINPUT_MODULES])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-libinput.pc
])

AC_OUTPUT
