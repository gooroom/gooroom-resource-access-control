AM_CPPFLAGS = 					\
	-I$(top_srcdir)				\
	-I../libgrac-common	\
	$(PLATFORM_CFLAGS)

lib_LTLIBRARIES = 	\
	libgrac-browser.la

libgrac_browser_headers =		\
	gac_control.h		\
	grac_browser.h		\
	grac_url.h		\
	grac_url_list.h		

libgrac_browser_sources =		\
	gac_control.c		\
	grac_browser.c		\
	grac_url.c		\
	grac_url_list.c		

libgrac_browser_includedir = $(includedir)/libgrac-browser

libgrac_browser_include_HEADERS =		\
	$(libgrac_browser_headers)

libgrac_browser_la_SOURCES =			\
	$(libgrac_browser_sources)		\
	$(libgrac_browser_headers)

libgrac_browser_la_CFLAGS = 			\
	$(GLIB_CFLAGS)			\
	$(LIBOOBS_CFLAGS)		\
	$(GOBJECT_CFLAGS)		\
	$(PLATFORM_CFLAGS)		\
	$(LIBXML_CFLAGS)

libgrac_browser_la_LDFLAGS =			\
	-no-undefined			\
	-export-dynamic			\
	-version-info $(GRAC_VERINFO)	\
	$(PLATFORM_LDFLAGS)

libgrac_browser_la_LIBADD = 		\
	$(GLIB_LIBS)		\
	$(LIBOOBS_LIBS)		\
	$(GOBJECT_LIBS)		\
	$(LIBXML_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgrac-browser.pc

