lib_LTLIBRARIES = libpepper-desktop-shell.la
include_HEADERS = pepper-desktop-shell.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-desktop-shell.pc

libpepper_desktop_shell_la_CFLAGS = $(PEPPER_DESKTOP_SHELL_CFLAGS)
libpepper_desktop_shell_la_LIBADD = $(PEPPER_DESKTOP_SHELL_LIBS)

libpepper_desktop_shell_la_SOURCES =            \
        desktop-shell-internal.h  \
        shell.c                   \
        shell-surface.c           \
        wl-shell.c

# xdg-shell.xml
xdg_shell_SOURCES  =                       \
        $(top_srcdir)/protocol/xdg-shell-protocol.c             \
        $(top_srcdir)/protocol/xdg-shell-server-protocol.h      \
        $(top_srcdir)/protocol/xdg-shell-client-protocol.h

BUILT_SOURCES = $(xdg_shell_SOURCES)
CLEANFILES = $(xdg_shell_SOURCES)

$(top_srcdir)/protocol/%-protocol.c : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) code < $< > $@

$(top_srcdir)/protocol/%-server-protocol.h : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) server-header < $< > $@

$(top_srcdir)/protocol/%-client-protocol.h : $(top_srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) client-header < $< > $@
