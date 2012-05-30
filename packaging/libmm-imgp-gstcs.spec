Name:       libmm-imgp-gstcs
Summary:    Multimedia Framework Utility Library
Version:    0.1
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: packaging/libmm-imgp-gstcs.manifest 
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common) 
#BuildRequires: libmm-common-internal-devel
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(gmodule-2.0)

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description
Multimedia Framework Utility Library

%package devel
Summary:    Multimedia Framework Utility Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
Development files for the Multimedia Framework Utility Library

%prep
%setup -q

./autogen.sh

CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed" \
./configure --prefix=%{_prefix}

%build
cp %{SOURCE1001} .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files
%manifest libmm-imgp-gstcs.manifest
%defattr(-,root,root,-)
%{_libdir}/*.so*
%exclude %{_includedir}/mmf/mm_util_gstcs.h
%exclude %{_libdir}/pkgconfig/mmutil-gstcs.pc
