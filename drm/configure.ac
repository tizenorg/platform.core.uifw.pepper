m4_define([pepper_drm_major], 0)
m4_define([pepper_drm_minor], 0)
m4_define([pepper_drm_micro], 0)

m4_define([pepper_drm_version], [pepper_drm_major.pepper_drm_minor.pepper_drm_micro])

AC_PREREQ([2.64])
AC_INIT([pepper_drm], [pepper_drm_version], [tkq.kim@samsung.com])

AC_SUBST([PEPPER_DRM_VERSION_MAJOR], [pepper_drm_major_version])
AC_SUBST([PEPPER_DRM_VERSION_MINOR], [pepper_drm_minor_version])
AC_SUBST([PEPPER_DRM_VERSION_MICRO], [pepper_drm_micro_version])
AC_SUBST([PEPPER_DRM_VERSION], [pepper_drm_version])

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PEPPER_DRM_MODULES="pepper pepper-render pepper-libinput pixman-1 wayland-server \
                    libdrm gbm pepper-desktop-shell"

PKG_CHECK_MODULES(PEPPER_DRM, [$PEPPER_DRM_MODULES])
AC_SUBST([PEPPER_DRM_REQUIRES], [$PEPPER_DRM_MODULES])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/pepper-drm.pc
])

AC_OUTPUT
