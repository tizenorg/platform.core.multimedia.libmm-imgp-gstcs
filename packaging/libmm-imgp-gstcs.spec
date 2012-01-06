#sbs-git:slp/pkgs/l/libmm-imgp-gstcs libmm-imgp-gstcs_0.1-1 9534d4f9e2abccae65989a77be0f7f1352bf2318
Name:       libmm-imgp-gstcs
Summary:    Multimedia Framework Utility Library
Version:    0.1
Release:    38
Group:      System/Libraries
License:    TBD
Source0:    %{name}-%{version}.tar.bz2
Requires(post):  /sbin/ldconfig
Requires(postun):  /sbin/ldconfig
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-log)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  pkgconfig(gstreamer-app-0.10)
BuildRequires:  pkgconfig(gmodule-2.0)

BuildRoot:  %{_tmppath}/%{name}-%{version}-build

%description

%package devel
Summary:    Multimedia Framework Utility Library (DEV)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel

%prep
%setup -q

./autogen.sh

CFLAGS="$CFLAGS -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" -D_MM_PROJECT_FLOATER" \
LDFLAGS+="-Wl,--rpath=%{_prefix}/lib -Wl,--hash-style=both -Wl,--as-needed" \
./configure --prefix=%{_prefix}

%build
make %{?jobs:-j%jobs}

sed -i -e "s#@GSTCS_REQPKG@#$GSTCS_REQPKG#g" gstcs/mmutil-gstcs.pc

%install
rm -rf %{buildroot}
%make_install

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/*.so*
#%exclude %{_bindir}/*_testsuite

%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/pkgconfig/*

