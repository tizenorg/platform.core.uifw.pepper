Name:		pepper
Version:	1.0.0
Release:	0
Summary:	Pepper - Library for developing wayland compositor
License:	MIT
Group:		Graphics & UI Framework/Wayland Window System

Source:		%{name}-%{version}.tar.xz

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

%description
Pepper is a lightweight and flexible library for developing various types of wayland compositors.

###### pepper-core
%package -n pepper-core
Summary: Core module for pepper package

%description -n pepper-core
This package includes core module and developer files common to all packages.

###### libinput
%package -n pepper-libinput
Summary: Libinput module for pepper package

%description -n pepper-libinput
This package includes libinput module files.

###### desktop-shell
%package -n pepper-desktop-shell
Summary: Desktop-shell module for pepper package

%description -n pepper-desktop-shell
This package includes desktop-shell module files.

###### render
%package -n pepper-render
Summary: Render module for pepper package

%description -n pepper-render
This package includes render module files.

###### drm backend
%package -n pepper-drm
Summary: Drm backend module for pepper package

%description -n pepper-drm
This package includes drm backend module files.

###### fbdev backend
%package -n pepper-fbdev
Summary: Fbdev backend module for pepper package

%description -n pepper-fbdev
This package includes fbdev backend module files.

###### wayland backend
%package -n pepper-wayland
Summary: Wayland backend module for pepper package

%description -n pepper-wayland
This package includes wayland backend module files.

###### doctor server
%package -n pepper-doctor
Summary: Doctor server for pepper package

%description -n pepper-doctor
This package includes doctor server files.

###### executing

%prep
%setup -q

%build
%autogen --disable-x11
make %{?_smp_mflags}

%install
%make_install

%post -n pepper-core -p /sbin/ldconfig
%postun -n pepper-core -p /sbin/ldconfig

%post -n pepper-libinput -p /sbin/ldconfig
%postun -n pepper-libinput -p /sbin/ldconfig

%post -n pepper-desktop-shell -p /sbin/ldconfig
%postun -n pepper-desktop-shell -p /sbin/ldconfig

%post -n pepper-render -p /sbin/ldconfig
%postun -n pepper-render -p /sbin/ldconfig

%post -n pepper-drm -p /sbin/ldconfig
%postun -n pepper-drm -p /sbin/ldconfig

%post -n pepper-fbdev -p /sbin/ldconfig
%postun -n pepper-fbdev -p /sbin/ldconfig

%post -n pepper-wayland -p /sbin/ldconfig
%postun -n pepper-wayland -p /sbin/ldconfig

%files -n pepper-core
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper.h
%{_includedir}/pepper/pepper-utils.h
%{_includedir}/pepper/pepper-output-backend.h
%{_includedir}/pepper/pepper-input-backend.h
%{_libdir}/libpepper.so*
%{_libdir}/pkgconfig/pepper.pc

%files -n pepper-libinput
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-libinput.h
%{_libdir}/libpepper-libinput.so*
%{_libdir}/pkgconfig/pepper-libinput.pc

%files -n pepper-desktop-shell
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-desktop-shell.h
%{_libdir}/libpepper-desktop-shell.so*
%{_libdir}/pkgconfig/pepper-desktop-shell.pc

%files -n pepper-render
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-render.h
%{_includedir}/pepper/pepper-*-renderer.h
%{_libdir}/libpepper-render.so*
%{_libdir}/pkgconfig/pepper-render.pc

%files -n pepper-drm
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-drm.h
%{_libdir}/libpepper-drm.so*
%{_libdir}/pkgconfig/pepper-drm.pc

%files -n pepper-fbdev
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-fbdev.h
%{_libdir}/libpepper-fbdev.so*
%{_libdir}/pkgconfig/pepper-fbdev.pc

%files -n pepper-wayland
%defattr(-,root,root,-)
%{_includedir}/pepper/pepper-wayland.h
%{_libdir}/libpepper-wayland.so*
%{_libdir}/pkgconfig/pepper-wayland.pc

%files -n pepper-doctor
%defattr(-,root,root,-)
%{_bindir}/doctor
