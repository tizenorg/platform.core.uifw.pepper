TESTPROGRAMS = drm-backend          \
               wayland-backend      \
               x11-backend          \
               simple-touch         \
               simple-shm

AM_CFLAGS = $(TEST_PROGRAM_CFLAGS)                      \
            -I$(top_srcdir)/src/                        \
            -I$(top_srcdir)/src/modules/wayland/        \
            -I$(top_srcdir)/src/modules/x11/            \
            -I$(top_srcdir)/src/modules/drm/            \
            -I$(top_srcdir)/src/modules/libinput/       \
            -I$(top_srcdir)/src/modules/desktop-shell

LDADD = $(top_builddir)/src/libpepper.la $(TEST_PROGRAM_LIBS)

noinst_PROGRAMS = $(TESTPROGRAMS)
TESTS = $(TESTPROGRAMS)
