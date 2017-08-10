AM_CPPFLAGS = 								\
	-I$(top_srcdir)							\
	$(PLATFORM_CFLAGS)

lib_LTLIBRARIES = 	\
	libgrac-common.la

libgrac_common_headers =		\
	sys_acl.h		\
	sys_cups.h		\
	sys_comm.h		\
	sys_user.h		\
	sys_user_list.h	\
	sys_ipset.h	\
	sys_ipt.h	\
	sys_iptables.h	\
	sys_file.h	\
	sys_etc.h	\
	cutility.h		\
	grm_log.h	\
	grac_config.h

libgrac_common_sources =		\
	cutility.c		\
	sys_acl.c		\
	sys_cups.c		\
	sys_comm.c		\
	sys_user.c		\
	sys_user_list.c	\
	sys_ipset.c	\
	sys_ipt.c	\
	sys_iptables.c	\
	sys_file.c	\
	sys_etc.c	\
	grm_log.c	\
	grac_config.c

libgrac_common_includedir = $(includedir)/libgrac-common

libgrac_common_include_HEADERS =		\
	$(libgrac_common_headers)

libgrac_common_la_SOURCES =			\
	$(libgrac_common_sources)		\
	$(libgrac_common_headers)

libgrac_common_la_CFLAGS = 			\
	$(GLIB_CFLAGS)			\
	$(LIBOOBS_CFLAGS)		\
	$(GOBJECT_CFLAGS)		\
	$(PLATFORM_CFLAGS)		\
	$(LIBXML_CFLAGS)

libgrac_common_la_LDFLAGS =					\
	-no-undefined					\
	-export-dynamic					\
	-version-info $(GRAC_VERINFO)	\
	$(PLATFORM_LDFLAGS)

libgrac_common_la_LIBADD = 		\
	$(GLIB_LIBS)		\
	$(LIBOOBS_LIBS)		\
	$(GOBJECT_LIBS)		\
	$(LIBXML_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgrac-common.pc
