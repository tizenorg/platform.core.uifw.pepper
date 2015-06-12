m4_define([pepper_fbdev_major], 0)
m4_define([pepper_fbdev_minor], 0)
m4_define([pepper_fbdev_micro], 0)

m4_define([pepper_fbdev_version], [pepper_fbdev_major.pepper_fbdev_minor.pepper_fbdev_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_fbdev], [pepper_fbdev_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_FBDEV_VERSION_MAJOR], [pepper_fbdev_major_version])
AC_SUBST([PEPPER_FBDEV_VERSION_MINOR], [pepper_fbdev_minor_version])
AC_SUBST([PEPPER_FBDEV_VERSION_MICRO], [pepper_fbdev_micro_version])
AC_SUBST([PEPPER_FBDEV_VERSION], [pepper_fbdev_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_FBDEV_MODULES="pepper pepper-render pepper-libinput pixman-1 wayland-server \
                    pepper-desktop-shell"

PKG_CHECK_MODULES(PEPPER_FBDEV, [$PEPPER_FBDEV_MODULES])
AC_SUBST([PEPPER_FBDEV_REQUIRES], [$PEPPER_FBDEV_MODULES])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-fbdev.pc
])

AC_OUTPUT
