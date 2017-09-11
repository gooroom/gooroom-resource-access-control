AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I../libgrac-common	\
	$(PLATFORM_CFLAGS)

lib_LTLIBRARIES = 	\
	libhook-clipboard.la

libhook_clipboard_includedir = \
	$(includedir)/libhook_clipboard

libhook_clipboard_la_SOURCES =			\
	hooker_clipboard.c

libhook_clipboard_la_CFLAGS = 			\
	$(GTK2_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(LIBOOBS_CFLAGS)		\
	$(GOBJECT_CFLAGS)		\
	$(PLATFORM_CFLAGS)		\
	$(LIBXML_CFLAGS)

libhook_clipboard_la_LDFLAGS =			\
	-no-undefined			\
	-export-dynamic			\
	-version-info $(GRAC_VERINFO)	\
	-shared -ldl -fPIC			\
	$(PLATFORM_LDFLAGS)

libhook_clipboard_la_LIBADD = 		\
	$(GLIB_LIBS)		\
	$(LIBOOBS_LIBS)		\
	$(GOBJECT_LIBS)		\
	$(LIBXML_LIBS)

#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = libhook-clipboard.pc

