lib_LTLIBRARIES = libpepper-libinput.la
include_HEADERS = pepper-libinput.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-libinput.pc

libpepper_libinput_la_CFLAGS = $(PEPPER_LIBINPUT_CFLAGS) -Wall
libpepper_libinput_la_LIBADD = $(PEPPER_LIBINPUT_LIBS)

libpepper_libinput_la_SOURCES = libinput-internal.h      \
                                libinput.c
