lib_LTLIBRARIES = libpepper-desktop-shell.la
include_HEADERS = pepper-desktop-shell.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-desktop-shell.pc

libpepper_desktop_shell_la_CFLAGS = $(PEPPER_DESKTOP_SHELL_CFLAGS)
libpepper_desktop_shell_la_LIBADD = $(PEPPER_DESKTOP_SHELL_LIBS)

libpepper_desktop_shell_la_SOURCES =    desktop-shell-internal.h  \
                                        shell.c                   \
                                        shell-surface.c           \
                                        wl-shell.c
