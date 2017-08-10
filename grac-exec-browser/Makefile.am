AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	-I../libgrac-browser	\
	-I../libgrac-common

#libexec_PROGRAMS = 		\
#	grac-exec-browser

sbin_PROGRAMS = 	grac-exec-browser

grac_exec_browser_SOURCES =	\
	grac_exec_browser.c

grac_exec_browser_CFLAGS =		\
	$(GLIB_CFLAGS)

grac_exec_browser_LDADD =	\
	$(GLIB_LIBS)				\
	-lpthread -lacl -lcups -lcrypt			\
	-ljson-c	\
	$(top_builddir)/libgrac-common/libgrac-common.la	\
	$(top_builddir)/libgrac-browser/libgrac-browser.la	


