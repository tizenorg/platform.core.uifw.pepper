lib_LTLIBRARIES = libpepper-render.la

AM_CFLAGS = $(GCC_CFLAGS)

libpepper_render_includedir=$(includedir)/pepper
libpepper_render_include_HEADERS = pepper-render.h

libpepper_render_la_CFLAGS = $(AM_CFLAGS) $(PEPPER_RENDER_CFLAGS)
libpepper_render_la_LIBADD = $(PEPPER_RENDER_LIBS)

libpepper_render_la_SOURCES = pepper-render.h           \
			      pepper-render-internal.h  \
			      renderer.c

# pixman renderer
libpepper_render_include_HEADERS += pepper-pixman-renderer.h
libpepper_render_la_SOURCES += pixman-renderer.c

# gl renderer
libpepper_render_include_HEADERS += pepper-gl-renderer.h
libpepper_render_la_SOURCES += gl-renderer.c    \
                               eglextwayland.h
