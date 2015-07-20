lib_LTLIBRARIES =
include_HEADERS =

# pepper-render library
lib_LTLIBRARIES += libpepper-render.la
include_HEADERS += pepper-render.h

libpepper_render_la_CFLAGS = $(PEPPER_RENDER_CFLAGS) $(PEPPER_INTERNAL_CFLAGS)

libpepper_render_la_LIBADD = $(PEPPER_RENDER_LIBS) $(PEPPER_INTERNAL_LIBS)

libpepper_render_la_SOURCES = pepper-render.h           \
			      pepper-render-internal.h  \
			      renderer.c

# pixman renderer
include_HEADERS += pepper-pixman-renderer.h
libpepper_render_la_SOURCES += pixman-renderer.c

# gl renderer
include_HEADERS += pepper-gl-renderer.h
libpepper_render_la_SOURCES += gl-renderer.c    \
                               eglextwayland.h
