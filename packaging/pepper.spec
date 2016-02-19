Name:		pepper
Version:	1.0.0
Release:	0
Summary:	Library for developing wayland compositor
License:	MIT
Group:		Graphics & UI Framework/Wayland Window System

Source:		%{name}-%{version}.tar.xz

%define ENABLE_TDM	1

BuildRequires:	autoconf > 2.64
BuildRequires:	automake >= 1.11
BuildRequires:	libtool >= 2.2
BuildRequires:	pkgconfig
BuildRequires:	xz
BuildRequires:	pkgconfig(wayland-server)
BuildRequires:	pkgconfig(pixman-1)
BuildRequires:	pkgconfig(libinput)
BuildRequires:	pkgconfig(libdrm)
BuildRequires:	pkgconfig(gbm)
BuildRequires:	pkgconfig(egl)
BuildRequires:	pkgconfig(glesv2)
BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:	doxygen
BuildRequires:	pkgconfig(wayland-tbm-client)
BuildRequires:  pkgconfig(wayland-tbm-server)
%if ("%{?tizen_target_name}" == "TM1")
BuildRequires:  pkgconfig(libdrm_sprd)
%endif
BuildRequires:  pkgconfig(libtbm)
%if "%{ENABLE_TDM}" == "1"
BuildRequires:  pkgconfig(libtdm)
%endif

%description
Pepper is a lightweight and flexible library for developing various types of wayland compositors.

###### pepper-devel
%package devel
Summary: Development module for pepper package
Requires: %{name} = %{version}-%{release}

%description devel
This package includes developer files common to all packages.

###### libinput
%package libinput
Summary: Libinput module for pepper package

%description libinput
This package includes libinput module files.

###### libinput-devel
%package libinput-devel
Summary: Libinput development module for pepper package
Requires: pepper-libinput = %{version}-%{release}

%description libinput-devel
This package includes libinput development module files.

###### desktop-shell
%package desktop-shell
Summary: Desktop-shell module for pepper package

%description desktop-shell
This package includes desktop-shell module files.

###### desktop-shell-devel
%package desktop-shell-devel
Summary: Desktop-shell development module for pepper package
Requires: pepper-desktop-shell = %{version}-%{release}

%description desktop-shell-devel
This package includes desktop-shell development module files.

###### render
%package render
Summary: Render module for pepper package

%description render
This package includes render module files.

###### render-devel
%package render-devel
Summary: Render development module for pepper package
Requires: pepper-render = %{version}-%{release}

%description render-devel
This package includes render development module files.

###### drm backend
%package drm
Summary: Drm backend module for pepper package

%description drm
This package includes drm backend module files.

###### drm backend devel
%package drm-devel
Summary: Drm backend development module for pepper package
Requires: pepper-drm = %{version}-%{release}

%description drm-devel
This package includes drm backend development module files.

###### tdm backend
%package tdm
Summary: TDM backend module for pepper package

%description tdm
This package includes tdm backend module files.

###### tdm backend devel
%package tdm-devel
Summary: TDM backend development module for pepper package
Requires: pepper-tdm = %{version}-%{release}

%description tdm-devel
This package includes drm backend development module files.

###### fbdev backend
%package fbdev
Summary: Fbdev backend module for pepper package

%description fbdev
This package includes fbdev backend module files.

###### fbdev backend devel
%package fbdev-devel
Summary: Fbdev backend development module for pepper package
Requires: pepper-fbdev = %{version}-%{release}

%description fbdev-devel
This package includes fbdev backend development module files.

###### wayland backend
%package wayland
Summary: Wayland backend module for pepper package

%description wayland
This package includes wayland backend module files.

###### wayland backend devel
%package wayland-devel
Summary: Wayland backend development module for pepper package
Requires: pepper-wayland = %{version}-%{release}

%description wayland-devel
This package includes wayland backend development module files.

###### doctor server
%package doctor
Summary: Doctor server for pepper package

%description doctor
This package includes doctor server files.

###### executing

%prep
%setup -q

%build
export CFLAGS="$(echo $CFLAGS | sed 's/-Wl,--as-needed//g')"
export CXXFLAGS="$(echo $CXXFLAGS | sed 's/-Wl,--as-needed//g')"
export FFLAGS="$(echo $FFLAGS | sed 's/-Wl,--as-needed//g')"

%autogen \
	--disable-x11 \
%if "%{ENABLE_TDM}" == "0"
	--disable-tdm \
%endif
	--enable-socket-fd=yes

make %{?_smp_mflags}

%install
%make_install

%post -n %{name} -p /sbin/ldconfig
%postun -n %{name} -p /sbin/ldconfig

%post libinput -p /sbin/ldconfig
%postun libinput -p /sbin/ldconfig

%post desktop-shell -p /sbin/ldconfig
%postun desktop-shell -p /sbin/ldconfig

%post render -p /sbin/ldconfig
%postun render -p /sbin/ldconfig

%post drm -p /sbin/ldconfig
%postun drm -p /sbin/ldconfig

%post fbdev -p /sbin/ldconfig
%postun fbdev -p /sbin/ldconfig

%post wayland -p /sbin/ldconfig
%postun wayland -p /sbin/ldconfig

%files -n %{name}
%defattr(-,root,root,-)
%{_libdir}/libpepper.so.*

%files devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper.h
%{_includedir}/pepper/pepper-utils.h
%{_includedir}/pepper/pepper-output-backend.h
%{_includedir}/pepper/pepper-input-backend.h
%{_libdir}/pkgconfig/pepper.pc
%{_libdir}/libpepper.so

%files libinput
%defattr(-,root,root,-)
%{_libdir}/libpepper-libinput.so.*

%files libinput-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-libinput.h
%{_libdir}/pkgconfig/pepper-libinput.pc
%{_libdir}/libpepper-libinput.so

%files desktop-shell
%defattr(-,root,root,-)
%{_libdir}/libpepper-desktop-shell.so.*
%{_bindir}/shell-client

%files desktop-shell-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-desktop-shell.h
%{_includedir}/pepper/pepper-shell-client-protocol.h
%{_includedir}/pepper/xdg-shell-client-protocol.h
%{_libdir}/pkgconfig/pepper-desktop-shell.pc
%{_libdir}/libpepper-desktop-shell.so

%files render
%defattr(-,root,root,-)
%{_libdir}/libpepper-render.so.*

%files render-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-render.h
%{_includedir}/pepper/pepper-*-renderer.h
%{_libdir}/pkgconfig/pepper-render.pc
%{_libdir}/libpepper-render.so

%files drm
%defattr(-,root,root,-)
%{_libdir}/libpepper-drm.so.*

%files drm-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-drm.h
%{_libdir}/pkgconfig/pepper-drm.pc
%{_libdir}/libpepper-drm.so

%if "%{ENABLE_TDM}" == "1"
%files tdm
%defattr(-,root,root,-)
%{_libdir}/libpepper-tdm.so.*

%files tdm-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-tdm.h
%{_libdir}/pkgconfig/pepper-tdm.pc
%{_libdir}/libpepper-tdm.so
%endif

%files fbdev
%defattr(-,root,root,-)
%{_libdir}/libpepper-fbdev.so.*

%files fbdev-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-fbdev.h
%{_libdir}/pkgconfig/pepper-fbdev.pc
%{_libdir}/libpepper-fbdev.so

%files wayland
%defattr(-,root,root,-)
%{_libdir}/libpepper-wayland.so.*

%files wayland-devel
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-wayland.h
%{_libdir}/pkgconfig/pepper-wayland.pc
%{_libdir}/libpepper-wayland.so

%files doctor
%defattr(-,root,root,-)
%{_bindir}/doctor
