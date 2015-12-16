lib_LTLIBRARIES = libpepper-desktop-shell.la
bin_PROGRAMS = shell-client
BUILT_SOURCES =
CLEANFILES =

AM_CFLAGS = $(GCC_CFLAGS)

BUILT_SOURCES += protocol/xdg-shell-protocol.c            \
                 protocol/xdg-shell-server-protocol.h     \
                 protocol/xdg-shell-client-protocol.h     \
                 protocol/pepper-shell-protocol.c         \
                 protocol/pepper-shell-server-protocol.h  \
                 protocol/pepper-shell-client-protocol.h

libpepper_desktop_includedir=$(includedir)/pepper
libpepper_desktop_include_HEADERS = pepper-desktop-shell.h                    \
                                    protocol/pepper-shell-client-protocol.h   \
                                    protocol/xdg-shell-client-protocol.h

libpepper_desktop_shell_la_CFLAGS = $(AM_CFLAGS)                    \
                                    -I$(srcdir)/protocol/           \
                                    $(PEPPER_DESKTOP_SHELL_CFLAGS)

libpepper_desktop_shell_la_LIBADD = $(PEPPER_DESKTOP_SHELL_LIBS)

libpepper_desktop_shell_la_SOURCES = desktop-shell-internal.h       \
                                     shell.c                        \
                                     shell-surface.c                \
                                     wl-shell.c                     \
                                     xdg-shell.c                    \
                                     pepper-shell.c                 \
                                     $(BUILT_SOURCES)

CLEANFILES += $(BUILT_SOURCES)

$(srcdir)/protocol/%-protocol.c : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) code < $< > $@

$(srcdir)/protocol/%-server-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) server-header < $< > $@

$(srcdir)/protocol/%-client-protocol.h : $(srcdir)/protocol/%.xml
	$(AM_V_GEN)$(wayland_scanner) client-header < $< > $@

shell_client_CFLAGS = $(SHELL_CLIENT_CFLAGS)
shell_client_LDADD = $(SHELL_CLIENT_LIBS)
shell_client_SOURCES = shell-client.c
