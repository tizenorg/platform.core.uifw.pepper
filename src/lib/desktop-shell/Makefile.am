lib_LTLIBRARIES = libpepper-desktop-shell.la
include_HEADERS = pepper-desktop-shell.h

libpepper_desktop_shell_la_CFLAGS = -I$(srcdir)/protocol/           \
                                    -I$(top_srcdir)/src/lib/pepper  \
                                    $(PIXMAN_CFLAGS)                \
                                    $(WAYLAND_SERVER_CFLAGS)

libpepper_desktop_shell_la_LIBADD = $(top_srcdir)/src/lib/pepper/libpepper.la

libpepper_desktop_shell_la_SOURCES = desktop-shell-internal.h                   \
                                     shell.c                                    \
                                     shell-surface.c                            \
                                     wl-shell.c                                 \
                                     $(srcdir)/protocol/xdg-shell-protocol.c    \
                                     xdg-shell.c

# xdg-shell.xml
xdg_shell_SOURCES  = $(srcdir)/protocol/xdg-shell-protocol.c        \
                     $(srcdir)/protocol/xdg-shell-server-protocol.h \
                     $(srcdir)/protocol/xdg-shell-client-protocol.h

BUILT_SOURCES = $(xdg_shell_SOURCES)
CLEANFILES = $(xdg_shell_SOURCES)

$(srcdir)/protocol/%-protocol.c : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) code < $< > $@

$(srcdir)/protocol/%-server-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) server-header < $< > $@

$(srcdir)/protocol/%-client-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) client-header < $< > $@
