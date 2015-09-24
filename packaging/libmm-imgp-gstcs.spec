#sbs-git:slp/pkgs/l/libmm-imgp-gstcs libmm-imgp-gstcs 0.1 62b62e6d483557fc5750d1b4986e9a98323f1194
Name:       libmm-imgp-gstcs
Summary:    Multimedia Framework Utility Library
Version:    0.6
Release:    16
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	libmm-imgp-gstcs.manifest
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gmodule-2.0)
BuildRequires:  pkgconfig(gstreamer-app-1.0)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-1.0)
BuildRequires:  pkgconfig(gstreamer-pbutils-1.0)


%description
Multimedia Framework Utility Library.

%package devel
Summary:    Multimedia Framework Utility Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Multimedia Framework Utility Library (DEV).

%prep
%setup -q
cp %{SOURCE1001} .

%build
./autogen.sh

export CFLAGS+=" -Wextra -Wno-array-bounds"
export CFLAGS+=" -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow"
export CFLAGS+=" -Wwrite-strings -Wswitch-default"
CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS+="-Wl,--rpath=%{_libdir} -Wl,--hash-style=both -Wl,--as-needed" \
%configure
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2.0 %{buildroot}/usr/share/license/%{name}

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
/usr/share/license/%{name}
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*
