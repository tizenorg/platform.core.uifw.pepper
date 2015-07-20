lib_LTLIBRARIES =
include_HEADERS =

# pepper-render library
lib_LTLIBRARIES += libpepper-render.la
include_HEADERS += pepper-render.h

libpepper_render_la_CFLAGS = -I$(top_srcdir)/src/lib/pepper

libpepper_render_la_LIBADD = $(top_srcdir)/src/lib/pepper/libpepper.la

libpepper_render_la_SOURCES = pepper-render.h           \
			      pepper-render-internal.h  \
			      renderer.c

# pixman renderer
include_HEADERS += pepper-pixman-renderer.h

libpepper_render_la_CFLAGS += $(PIXMAN_CFLAGS)
libpepper_render_la_LIBADD += $(PIXMAN_LIBS)

libpepper_render_la_SOURCES += pixman-renderer.c

# gl renderer
include_HEADERS += pepper-gl-renderer.h

libpepper_render_la_CFLAGS += $(EGL_CFLAGS) $(GLESV2_CLFAGS)
libpepper_render_la_LIBADD += $(EGL_LIBS) $(GLESV2_LIBS)

libpepper_render_la_SOURCES += gl-renderer.c    \
                               eglextwayland.h
