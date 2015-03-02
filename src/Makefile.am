bin_PROGRAMS =
lib_LTLIBRARIES =

# pepper library
lib_LTLIBRARIES += libpepper.la
libpepper_la_CFLAGS = $(PEPPER_CFLAGS)
libpepper_la_LIBADD = $(PEPPER_LIBS)
libpepper_la_SOURCES = pepper.h			\
		       pepper_internal.h	\
		       common.h			\
		       compositor.c		\
		       output.c			\
		       client.c			\
		       surface.c

# Pepper server executable
bin_PROGRAMS += pepper

pepper_LDFLAGS = -export-dynamic
pepper_LDADD = libpepper.la
pepper_SOURCES = server/server.c
