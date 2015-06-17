m4_define([pepper_desktop_shell_major], 0)
m4_define([pepper_desktop_shell_minor], 0)
m4_define([pepper_desktop_shell_micro], 0)

m4_define([pepper_desktop_shell_version],
          [pepper_desktop_shell_major.pepper_desktop_shell_minor.pepper_desktop_shell_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_desktop_shell], [pepper_desktop_shell_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_DESKTOP_SHELL_VERSION_MAJOR], [pepper_desktop_shell_major_version])
AC_SUBST([PEPPER_DESKTOP_SHELL_VERSION_MINOR], [pepper_desktop_shell_minor_version])
AC_SUBST([PEPPER_DESKTOP_SHELL_VERSION_MICRO], [pepper_desktop_shell_micro_version])
AC_SUBST([PEPPER_DESKTOP_SHELL_VERSION], [pepper_desktop_shell_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_DESKTOP_SHELL_MODULES="pepper"
PKG_CHECK_MODULES(PEPPER_DESKTOP_SHELL, [$PEPPER_DESKTOP_SHELL_MODULES])
AC_SUBST([PEPPER_DESKTOP_SHELL_REQUIRES], [$PEPPER_DESKTOP_SHELL_MODULES])

# wayland-scanner
AC_PATH_PROG([wayland_scanner], [wayland-scanner])
if test x$wayland_scanner = x; then
    PKG_CHECK_MODULES(WAYLAND_SCANNER, [wayland-scanner])
    wayland_scanner=`$PKG_CONFIG --variable=wayland_scanner wayland-scanner`
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-desktop-shell.pc
])

AC_OUTPUT
