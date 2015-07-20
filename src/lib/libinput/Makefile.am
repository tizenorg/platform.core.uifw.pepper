lib_LTLIBRARIES = libpepper-libinput.la
include_HEADERS = pepper-libinput.h

libpepper_libinput_la_CFLAGS = $(PEPPER_LIBINPUT_CFLAGS)    \
                               $(PEPPER_INTERNAL_CFLAGS)

libpepper_libinput_la_LIBADD = $(PEPPER_LIBINPUT_LIBS)  \
                               $(PEPPER_INTERNAL_LIBS)

libpepper_libinput_la_SOURCES = libinput-internal.h \
                                libinput.c
