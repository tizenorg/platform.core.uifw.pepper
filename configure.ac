#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([pepper_major], 0)
m4_define([pepper_minor], 0)
m4_define([pepper_micro], 0)

m4_define([pepper_version], [pepper_major.pepper_minor.pepper_micro])

AC_PREREQ([2.64])
AC_INIT([pepper], [pepper_version], [tkq.kim@samsung.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC

# libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Pepper build requirements
LIB_PEPPER_MODULES="wayland-server >= 1.6.00 pixman-1 >= 0.25.2"

# Check pepper modules
PKG_CHECK_MODULES(LIB_PEPPER, [$LIB_PEPPER_MODULES])

# gl renderer
AC_ARG_ENABLE(gl-renderer,
              AC_HELP_STRING([--enable-gl-renderer], [enable OpenGL renderer]),
              [enable_gl_renderer=$enableval], [enable_gl_renderer=yes])

AM_CONDITIONAL(ENABLE_GL_RENDERER, test $enable_gl_renderer = yes)

if test $enable_gl_renderer = yes; then
    AC_DEFINE(ENABLE_GL_RENDERER, 1, [Enable OpenGL renderer])
    PKG_CHECK_MODULES(GL_RENDERER, [egl glesv2])
fi

# wayland backend
AC_ARG_ENABLE(wayland-backend,
              AC_HELP_STRING([--enable-wayland-backend], [enable wayland backend]),
              [enable_wayland_backend=$enableval], [enable_wayland_backend=yes])

AM_CONDITIONAL(ENABLE_WAYLAND_BACKEND, test $enable_wayland_backend = yes)

if test $enable_wayland_backend = yes; then
    AC_DEFINE(ENABLE_WAYLAND_BACKEND, 1, [Enable wayland backend])
    PKG_CHECK_MODULES(WAYLAND_BACKEND, [wayland-client >= 1.6.00])
fi

AC_ARG_ENABLE(wayland-backend-egl,
              AC_HELP_STRING([--enable-wayland-backend-egl], [enable wayland backend EGL support]),
              [if test $enableval = yes && test $enable_wayland_backend = yes; then
                enable_wayland_backend_egl=yes
               else
                enable_wayland_backend_egl=no
               fi],
              [enable_wayland_backend_egl=no])

AM_CONDITIONAL(ENABLE_WAYLAND_BACKEND_EGL, test $enable_wayland_backend_egl = yes)

if test $enable_wayland_backend_egl = yes; then
    AC_DEFINE(ENABLE_WAYLAND_BACKEND_EGL, 1, [Enable wayland backend EGL support])
    PKG_CHECK_MODULES(WAYLAND_BACKEND, [wayland-egl])
fi

# drm backend
AC_ARG_ENABLE(drm-backend,
              AC_HELP_STRING([--enable-drm-backend], [enable drm backend]),
              [enable_drm_backend=$enableval], [enable_drm_backend=yes])

AM_CONDITIONAL(ENABLE_DRM_BACKEND, test $enable_drm_backend = yes)

if test $enable_drm_backend = yes; then
    AC_DEFINE(ENABLE_DRM_BACKEND, 1, [Enable drm backend])
    PKG_CHECK_MODULES(DRM_BACKEND, [libinput >= 0.6.0])
fi

# x11 backend
AC_ARG_ENABLE(x11-backend,
              AC_HELP_STRING([--enable-x11-backend], [enable x11 backend]),
              [enable_x11_backend=$enableval], [enable_x11_backend=yes])
#              [enable_x11_backend=$enableval], [enable_x11_backend=no])

AM_CONDITIONAL(ENABLE_X11_BACKEND, test $enable_x11_backend = yes)

if test $enable_x11_backend = yes; then
    AC_DEFINE(ENABLE_X11_BACKEND, 1, [Enable x11 backend])
    PKG_CHECK_MODULES(X11_BACKEND, [x11 xcb-shm x11-xcb])
fi

# pepper server
AC_ARG_ENABLE(pepper-server,
              AC_HELP_STRING([--enable-pepper-server], [build pepper reference server]),
              [enable_pepper_server=$enableval], [enable_pepper_server=yes])

AM_CONDITIONAL(ENABLE_PEPPER_SERVER, test $enable_pepper_server = yes)

if test $enable_pepper_server = yes; then
    AC_DEFINE(ENABLE_PEPPER_SERVER, 1, [Build pepper reference server])
    PKG_CHECK_MODULES(PEPPER_SERVER, [wayland-server >= 1.6.00])
fi

# Output files
AC_CONFIG_FILES([
Makefile
shared/Makefile
src/Makefile
test/Makefile
])

AC_OUTPUT
