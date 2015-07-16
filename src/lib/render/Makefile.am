lib_LTLIBRARIES =
include_HEADERS =

# pepper-render library
lib_LTLIBRARIES += libpepper-render.la
include_HEADERS += pepper-render.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pepper-render.pc

libpepper_render_la_CFLAGS = $(PEPPER_RENDER_CFLAGS) -Wall \
                             -I$(top_srcdir)/src/lib/pepper
libpepper_render_la_LIBADD = $(PEPPER_RENDER_LIBS)

libpepper_render_la_SOURCES = pepper-render.h		\
			      pepper-render-internal.h	\
			      renderer.c

# pixman renderer
if ENABLE_PIXMAN
include_HEADERS += pepper-pixman-renderer.h

libpepper_render_la_CFLAGS += $(PIXMAN_CFLAGS)
libpepper_render_la_LIBADD += $(PIXMAN_LIBS)

libpepper_render_la_SOURCES += pixman-renderer.c
endif

# gl renderer
if ENABLE_GL
include_HEADERS += pepper-gl-renderer.h

libpepper_render_la_CFLAGS += $(GL_CFLAGS)
libpepper_render_la_LIBADD += $(GL_LIBS)

libpepper_render_la_SOURCES += gl-renderer.c    \
                               eglextwayland.h
endif
