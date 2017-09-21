#define _GNU_SOURCE

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

//#include <libnotify/notify.h>

static unsigned char *conf_path="/etc/gooroom/grac.d/hook-screenshooter.conf";

//static unsigned char *alert_msg="Screen Capture is disallowed!!";
#define alert_msg "Screen Capture is disallowed!!"

static void notify(char *message)
{
	// log message 
	syslog (LOG_WARNING, "Gooroom Resource Access Control : %s", message);

	// alert window
	execl("/usr/bin/notify-send", "/usr/bin/notify-send", "Gooroom Resource Access Control", message, NULL);

/*
	GError *error = NULL;
	notify_init("GRAC warining");
	NotifyNotification *noti = notify_notification_new("Gooroom Resource Access Control", message, NULL);
	if (notify_notification_show(noti, &error) == FALSE) {
		FILE *fp = fopen("/tmp/hook_scrn.error", "a");
		fprintf(fp, "%s\n", error->message);
		fclose(fp);
		g_error_free(error);
	}
	g_object_unref(G_OBJECT(noti));
	notify_uninit();
*/

/*
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
*/
}

static	int	 disallow_function()
{
	FILE *confp;
	int	status = -1;
	int	result = 0;

	confp = fopen(conf_path, "r");
	if(confp == NULL)
		return 0;

	fscanf(confp, "%d", &status);
	fclose(confp);

	if (status == 1) {
		notify(alert_msg);
		result = 1;
	}

	return result;
}


// Against xfce4-screenshooter

GdkPixbuf *gdk_pixbuf_get_from_drawable (GdkPixbuf   *dest,
                                         GdkDrawable *src,
                                         GdkColormap *cmap,
                                         int          src_x,
                                         int          src_y,
                                         int          dest_x,
                                         int          dest_y,
                                         int          width,
                                         int          height)
{

	if (disallow_function()) {
		exit(1);
		//return NULL;
	}

	GdkPixbuf *(*original_func)(GdkPixbuf *, GdkDrawable *, GdkColormap *, int, int, int, int, int, int);

	original_func = dlsym(RTLD_NEXT, "gdk_pixbuf_get_from_drawable");
	return (*original_func)(dest, src, cmap, src_x, src_y, dest_x, dest_y, width, height);
}


// Against gnome-screenshot
GdkPixbuf *gdk_pixbuf_get_from_window (GdkWindow *window, gint src_x, gint src_y, gint width, gint height)
{
	if (disallow_function()) {
		//exit(1);
		return NULL;
	}

	GdkPixbuf *(*original_func)(GdkWindow *, gint, gint, gint, gint);

	original_func = dlsym(RTLD_NEXT, "gdk_pixbuf_get_from_window");
	return (*original_func)(window, src_x, src_y, width, height);
}


// Agaist xwd, X11-Apps
XImage *XGetImage(Display *display, 
                  Drawable d, 
                  int x, int y, 
                  unsigned int width, unsigned int height,
                  unsigned long plane_mask,
                  int format)
{
	if (disallow_function()) {
		//exit(1);
		return NULL;
	}

	XImage *(*original_func)(Display *, Drawable, int, int, unsigned int, unsigned int, unsigned long, int);

	original_func = dlsym(RTLD_NEXT, "XGetImage");
	return (*original_func)(display, d, x, y, width, height, plane_mask, format);
}

