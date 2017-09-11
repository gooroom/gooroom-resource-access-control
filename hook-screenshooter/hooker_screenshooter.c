#define _GNU_SOURCE

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

//static unsigned char *conf_path="/home/ultract/gooroom-resource-access-control/hook_screenshooter/hooker.conf";
static unsigned char *conf_path="/etc/gooroom/hook_screenshooter.conf";

//static unsigned char *alert_msg="Screen Capture is disallowed!!";
#define alert_msg "Screen Capture is disallowed!!"

// Against xfce4-screenshooter

GdkPixbuf *gdk_pixbuf_get_from_drawable (GdkPixbuf   *dest,
                                         GdkDrawable *src,
                                         GdkColormap *cmap,
                                         int          src_x,
                                         int          src_y,
                                         int          dest_x,
                                         int          dest_y,
                                         int          width,
                                         int          height){

	GdkPixbuf *(*original_func)(GdkPixbuf *, GdkDrawable *, GdkColormap *, int, int, int, int, int, int);
	FILE *confp;
	int status = -1;

	GtkWidget *dialog;

	original_func = dlsym(RTLD_NEXT, "gdk_pixbuf_get_from_drawable");

	confp = fopen(conf_path, "r");
	if(confp == NULL) {
		return (*original_func)(dest, src, cmap, src_x, src_y, dest_x, dest_y, width, height);
	}

	fscanf(confp, "%d", &status);
	fclose(confp);

	if(status != 0){
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		
		exit(1);
		//return NULL;
	}
	
	return (*original_func)(dest, src, cmap, src_x, src_y, dest_x, dest_y, width, height);
}


// Against gnome-screenshot
GdkPixbuf *gdk_pixbuf_get_from_window (GdkWindow *window, gint src_x, gint src_y, gint width, gint height){

	GdkPixbuf *(*original_func)(GdkWindow *, gint, gint, gint, gint);

	FILE *confp;
	int status;

	GtkWidget *dialog;

	original_func = dlsym(RTLD_NEXT, "gdk_pixbuf_get_from_window");

	confp = fopen(conf_path, "r");
	if(confp == NULL)
		return (*original_func)(window, src_x, src_y, width, height);

	fscanf(confp, "%d", &status);
	fclose(confp);
	
	if(status != 0){
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		
		//exit(1);
		return NULL;
	}
	return (*original_func)(window, src_x, src_y, width, height);
}


// Agaist xwd, X11-Apps
XImage *XGetImage(Display *display, 
                  Drawable d, 
                  int x, int y, 
                  unsigned int width, unsigned int height,
                  unsigned long plane_mask,
                  int format){

	XImage *(*original_func)(Display *, Drawable, int, int, unsigned int, unsigned int, unsigned long, int);

	FILE *confp;
	int status;

	//GtkWidget *dialog;

	original_func = dlsym(RTLD_NEXT, "XGetImage");

	confp = fopen(conf_path, "r");
	if(confp == NULL)
		return (*original_func)(display, d, x, y, width, height, plane_mask, format);

	fscanf(confp, "%d", &status);
	fclose(confp);
	
	if(status != 0){
		//dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg);
		//gtk_dialog_run(GTK_DIALOG(dialog));
		//gtk_widget_destroy(dialog);
		printf("%s\n", alert_msg);
		//exit(1);
		return NULL;
	}
	
	return (*original_func)(display, d, x, y, width, height, plane_mask, format);
}

