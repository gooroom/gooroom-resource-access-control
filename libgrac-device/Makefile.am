AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I../libgrac-common	\
	$(PLATFORM_CFLAGS)	

lib_LTLIBRARIES = 	\
	libgrac-device.la

libgrac_device_headers =	\
	grac_rule.h			\
	grac_rule_network.h	\
	grac_rule_udev.h		\
	grac_rule_hook.h		\
	grac_rule_printer_cups.h	\
	grac_resource.h		\
	grac_adjust_udev_rule.h

libgrac_device_sources =	\
	grac_rule.c			\
	grac_rule_network.c	\
	grac_rule_udev.c		\
	grac_rule_hook.c		\
	grac_rule_printer_cups.c	\
	grac_resource.c		\
	grac_adjust_udev_rule.c

libgrac_device_includedir = \
	$(includedir)/libgrac-device

libgrac_device_include_HEADERS =		\
	$(libgrac_device_headers)

libgrac_device_la_SOURCES =			\
	$(libgrac_device_sources)		\
	$(libgrac_device_headers)

libgrac_device_la_CFLAGS = 			\
	$(GLIB_CFLAGS)			\
	$(LIBOOBS_CFLAGS)		\
	$(GOBJECT_CFLAGS)		\
	$(PLATFORM_CFLAGS)		\
	$(LIBXML_CFLAGS)	

libgrac_device_la_LDFLAGS =			\
	-no-undefined			\
	-export-dynamic			\
	-version-info $(GRAC_VERINFO)	\
	$(PLATFORM_LDFLAGS)

libgrac_device_la_LIBADD = 		\
	$(GLIB_LIBS)		\
	$(LIBOOBS_LIBS)		\
	$(GOBJECT_LIBS)		\
	$(LIBXML_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgrac-device.pc

