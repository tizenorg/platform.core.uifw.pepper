bin_PROGRAMS =
lib_LTLIBRARIES =
moduledir = $(libdir)/pepper
module_LTLIBRARIES =

# pepper library
lib_LTLIBRARIES += libpepper.la
libpepper_la_CFLAGS = $(PEPPER_CFLAGS) -DMODULEDIR='"$(moduledir)"'
libpepper_la_LIBADD = $(PEPPER_LIBS)
libpepper_la_SOURCES = pepper.h                 \
                       pepper_internal.h        \
                       common.h                 \
                       common.c                 \
                       compositor.c             \
                       output.c                 \
                       input.c                  \
                       client.c                 \
                       shell.c                  \
                       surface.c                \
                       debug_ch.c               \
                       log.c

# input module library
module_LTLIBRARIES += input_libinput.la
input_libinput_la_LDFLAGS = -module -avoid-version
input_libinput_la_CFLAGS = $(PEPPER_CFLAGS) $(LIBINPUT_CFLAGS)
input_libinput_la_LIBADD = $(PEPPER_LIBS) $(LIBINPUT_LIBS)
input_libinput_la_SOURCES = modules/input_libinput.c

# Pepper server executable
bin_PROGRAMS += pepper

pepper_LDFLAGS = -export-dynamic
pepper_LDADD = libpepper.la -ldl
pepper_SOURCES = server/server.c
