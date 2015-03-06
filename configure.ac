#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([pepper_major], 0)
m4_define([pepper_minor], 0)
m4_define([pepper_micro], 0)

m4_define([pepper_version], [pepper_major.pepper_minor.pepper_micro])

AC_PREREQ([2.64])
AC_INIT([pepper], [pepper_version], [tkq.kim@samsung.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC

# libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Pepper build requirements
PEPPER_MODULES="wayland-server >= 1.6.00 pixman-1 >= 0.25.2"

# Check pepper modules
PKG_CHECK_MODULES(PEPPER, [$PEPPER_MODULES])

# Check libinput
PKG_CHECK_MODULES(LIBINPUT, [libinput >= 0.6.0])

# Check pepper server modules
# Output files
AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT
