AM_CPPFLAGS =			\
	-I$(top_srcdir)		\
	-I/usr/include/gooroom/libgrmpycaller	\
	-I../libgrac-common	\
	-I../libgrac-device	

libexec_PROGRAMS = \
	grac-device-daemon

#sbin_PROGRAMS = 			\
#	grac-device-daemon

#grac_device_daemon_HEADERS =	\
#	grac_device_daemon.h	

grac_device_daemon_SOURCES =	\
	grac_device_daemon.c

grac_device_daemon_CFLAGS =		\
	$(GLIB_CFLAGS)		\
	$(GIO_UNIX_CFLAGS)

grac_device_daemon_LDADD =	\
	$(GLIB_LIBS)		\
	$(GIO_UNIX_LIBS)	\
	-lacl		\
	-lcups		\
	-ljson-c	\
	-lgrmpycaller	\
	-ludev	\
	$(top_builddir)/libgrac-common/libgrac-common.la	\
	$(top_builddir)/libgrac-device/libgrac-device.la


