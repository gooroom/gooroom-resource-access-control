AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I../libgrac-common	\
	$(PLATFORM_CFLAGS)

lib_LTLIBRARIES = 	\
	libgrac-device.la

libgrac_device_headers =		\
	grac_map.h		\
	grac_apply.h			\
	grac_apply_network.h	\
	grac_apply_printer.h		\
	grac_apply_usb.h		\
	grac_apply_capture.h 	\
	grac_apply_dir.h		\
	grac_access.h		\
	grac_access_attr.h	

libgrac_device_sources =		\
	grac_map.c		\
	grac_apply.c			\
	grac_apply_network.c	\
	grac_apply_printer.c		\
	grac_apply_usb.c		\
	grac_apply_capture.c 	\
	grac_apply_dir.c		\
	grac_access.c		\
	grac_access_attr.c	

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

