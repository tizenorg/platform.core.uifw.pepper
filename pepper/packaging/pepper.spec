Name:		pepper
Version:	1.0.0
Release:	0
Summary:	Pepper - Library for developing wayland compositor
License:	Proprietary
Group:		Graphics & UI Framework/Wayland Window System

Source:		%{name}-%{version}.tar.xz

BuildRequires:	autoconf > 2.64
BuildRequires:	automake >= 1.11
BuildRequires:	libtool >= 2.2
BuildRequires:	pkgconfig
BuildRequires:	xz
BuildRequires:	pkgconfig(wayland-server)
BuildRequires:	pkgconfig(pixman-1)

%description
Pepper is a lightweight and flexible library for developing various types of wayland compositors.

%prep
%setup -q

%build
%autogen

make %{?_smp_mflags}

%install
%make_install

%files
%{_libdir}/libpepper.so*

%post
