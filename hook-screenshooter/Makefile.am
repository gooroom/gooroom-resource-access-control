AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I../libgrac-common	\
	$(PLATFORM_CFLAGS)	

lib_LTLIBRARIES = 	\
	libhook-screenshooter.la

libhook_screenshooter_includedir = \
	$(includedir)/libhook_screenshooter

libhook_screenshooter_la_SOURCES =			\
	hooker_screenshooter.c

libhook_screenshooter_la_CFLAGS = 			\
	$(GTK2_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(LIBOOBS_CFLAGS)		\
	$(GOBJECT_CFLAGS)		\
	$(PLATFORM_CFLAGS)		\
	$(LIBXML_CFLAGS)		

libhook_screenshooter_la_LDFLAGS =			\
	-no-undefined			\
	-export-dynamic			\
	-version-info $(GRAC_VERINFO)	\
   -shared -ldl -fPIC 			\
	$(PLATFORM_LDFLAGS)

libhook_screenshooter_la_LIBADD = 		\
	$(GLIB_LIBS)		\
	$(LIBOOBS_LIBS)		\
	$(GOBJECT_LIBS)		\
	$(LIBXML_LIBS)

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = libhook-screenshooter.pc

