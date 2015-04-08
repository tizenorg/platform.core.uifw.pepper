TESTPROGRAMS = wayland-backend

AM_CFLAGS = -I$(top_srcdir)/src/                        \
            -I$(top_srcdir)/src/modules/wayland/        \
            -I$(top_srcdir)/src/modeuls/x11/            \
            -I$(top_srcdir)/src/modules/drm/

LDADD = $(top_builddir)/src/libpepper.la $(LIB_PEPPER_LIBS)

noinst_PROGRAMS = $(TESTPROGRAMS)
TESTS = $(TESTPROGRAMS)
