/*
 * test_notify.c
 *
 *  Created on: 2017. 9. 14.
 *      Author: yang
 */


#include <stdio.h>
#include <string.h>
#include <libnotify/notify.h>

void notify(char *msg)
{
	notify_init("GRAC warining");
	NotifyNotification *noti = notify_notification_new("Hello", msg, "this is a test");
	notify_notification_show(noti, NULL);
	g_object_unref(G_OBJECT(noti));
	notify_uninit();
}

void t_notify()
{
	char buf[1024];

	while (1) {
		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		if (strlen(buf) <= 1)
			break;
		notify(buf);
	}
}

