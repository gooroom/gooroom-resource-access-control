dbusconfdir   = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = kr.gooroom.GRACDEVD.conf

gracdefaultdir   = /etc/gooroom/grac.d
gracdefault_DATA = default.rules

gracmapdir   = /etc/gooroom/grac.d
gracmap_DATA = grac-udev-rules.map

servicedir       = $(datadir)/dbus-1/system-services
service_in_files = kr.gooroom.GRACDEVD.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< >$@

systemdsystemunitdir = /lib/systemd/system
systemdsystemunit_DATA = grac-device-daemon.service

grac-device-daemon.service: grac-device-daemon.service.in
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< >$@

EXTRA_DIST = 			\
	$(dbusconf_DATA)	\
	$(gracdefault_DATA)	\
	$(gracmap_DATA)	\
	$(service_in_files)	\
	grac-device-daemon.service.in 

#	$(dbusif_DATA)

DISTCLEANFILES = $(service_DATA)

CLEANFILES = grac-device-daemon.service
