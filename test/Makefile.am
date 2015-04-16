TESTPROGRAMS = wayland-backend      \
               simple-touch         \
               simple-shm

AM_CFLAGS = $(TEST_PROGRAM_CFLAGS)                      \
            -I$(top_srcdir)/src/                        \
            -I$(top_srcdir)/src/modules/wayland/        \
            -I$(top_srcdir)/src/modeuls/x11/            \
            -I$(top_srcdir)/src/modules/drm/

LDADD = $(top_builddir)/src/libpepper.la $(TEST_PROGRAM_LIBS)

noinst_PROGRAMS = $(TESTPROGRAMS)
TESTS = $(TESTPROGRAMS)
