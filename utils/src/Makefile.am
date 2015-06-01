lib_LTLIBRARIES = libpepper-utils.la
include_HEADERS = pepper-utils.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-utils.pc

libpepper_utils_la_CFLAGS = $(PEPPER_UTILS_CFLAGS)
libpepper_utils_la_LIBADD = $(PEPPER_UTILS_LIBS)

libpepper_utils_la_SOURCES = pepper-utils.h	\
                             shm.c
