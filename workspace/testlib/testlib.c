/*
 * testlib.c
 *
 *  Created on: 2017. 10. 23.
 *      Author: yang
 */


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

#define alert_msg "Screen Capture is disallowed!!"


static void test(const char *func)
{
	syslog (LOG_WARNING, "Gooroom Resource Access Control xyz : %s %d %d", func, getpid(), getppid());
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
	test(__FUNCTION__);

	GdkPixbuf *(*original_func)(GdkPixbuf *, GdkDrawable *, GdkColormap *, int, int, int, int, int, int);
	original_func = dlsym(RTLD_NEXT, "gdk_pixbuf_get_from_drawable");
	return (*original_func)(dest, src, cmap, src_x, src_y, dest_x, dest_y, width, height);
}


// Against gnome-screenshot
GdkPixbuf *gdk_pixbuf_get_from_window (GdkWindow *window, gint src_x, gint src_y, gint width, gint height)
{
	test(__FUNCTION__);

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
	test(__FUNCTION__);

	XImage *(*original_func)(Display *, Drawable, int, int, unsigned int, unsigned int, unsigned long, int);

	original_func = dlsym(RTLD_NEXT, "XGetImage");
	return (*original_func)(display, d, x, y, width, height, plane_mask, format);
}

