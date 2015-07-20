bin_PROGRAMS = doctor
doctor_CFLAGS = -I$(top_srcdir)/src/lib/pepper $(PIXMAN_CFLAGS) $(WAYLAND_SERVER_CFLAGS)
doctor_LDADD = $(top_srcdir)/src/lib/pepper/libpepper.la $(WAYLAND_SERVER_LIBS)
doctor_SOURCES = server.c
