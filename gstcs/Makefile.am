ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmutil_imgp_gstcs.la
#bin_PROGRAMS = mmutil_gstcs 

installmmutil_gstcsdir = $(includedir)/mmf
installmmutil_gstcs_HEADERS = include/mm_util_gstcs.h
noinst_HEADERS = include/mm_util_gstcs_internal.h

libmmutil_imgp_gstcs_la_SOURCES = mm_util_gstcs.c
	
libmmutil_imgp_gstcs_la_CFLAGS = -I$(srcdir)/include \
 	                     $(MMCOMMON_CFLAGS) \
			     $(GLIB_CFLAGS) \
			     $(GST_CFLAGS) \
			     $(GSTAPP_CFLAGS)  \
                             $(MMLOG_CFLAGS) -DMMF_LOG_OWNER=0x0100 -DMMF_DEBUG_PREFIX=\"MMF-IMAGE\"

libmmutil_imgp_gstcs_la_LIBADD = $(MMCOMMON_LIBS) \
			    $(GLIB_LIBS) \
			    $(GST_LIBS) \
			    $(GSTAPP_LIBS) \
			    $(MMLOG_LIBS)

pcfiles = mmutil-gstcs.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
SUBDIRS = . 
