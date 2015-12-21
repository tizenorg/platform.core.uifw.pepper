lib_LTLIBRARIES = libpepper-tdm.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_tdm_includedir=$(includedir)/pepper
libpepper_tdm_include_HEADERS = pepper-tdm.h

libpepper_tdm_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_TDM_CFLAGS)
libpepper_tdm_la_LIBADD = $(PEPPER_TDM_LIBS)

libpepper_tdm_la_SOURCES =  tdm-internal.h  \
                            tdm-common.c    \
                            tdm-output.c
