lib_LTLIBRARIES = libpepper-libinput.la
include_HEADERS = pepper-libinput.h

libpepper_libinput_la_CFLAGS = $(LIBINPUT_CFLAGS)               \
                               $(PIXMAN_CFLAGS)                 \
                               -I$(top_srcdir)/src/lib/pepper

libpepper_libinput_la_LIBADD = $(LIBINPUT_LIBS)

libpepper_libinput_la_SOURCES = libinput-internal.h \
                                libinput.c
