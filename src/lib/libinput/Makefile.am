lib_LTLIBRARIES = libpepper-libinput.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_libinput_includedir=$(includedir)/pepper
libpepper_libinput_include_HEADERS = pepper-libinput.h

libpepper_libinput_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_LIBINPUT_CFLAGS)
libpepper_libinput_la_LIBADD = $(PEPPER_LIBINPUT_LIBS)

libpepper_libinput_la_SOURCES = libinput-internal.h \
                                libinput.c
