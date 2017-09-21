#define _GNU_SOURCE

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>

#include <uim/uim.h>

static unsigned char *conf_path="/etc/gooroom/grac.d/hook-clipboard.conf";

static unsigned char *alert_msg_copy="Clipboard(copy) is disallowed!!";
static unsigned char *alert_msg_paste="Clipboard(paste) is disallowed!!";

static void notify(char *message)
{
	// log message 
	syslog (LOG_WARNING, "Gooroom Resource Access Control : %s", message);

	// alert window
	execl("/usr/bin/notify-send", "/usr/bin/notify-send", "Gooroom Resource Access Control", message, NULL);
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
		result = 1;
	}

	return result;
}


// Hook "Copy" Operation, gtk based application
/*
void
gdk_selection_send_notify_for_display (GdkDisplay *display,
                                       GdkNativeWindow requestor,
                                       GdkAtom selection,
                                       GdkAtom target,
                                       GdkAtom property,
                                       guint32 time_)
{
	if (disallow_function()) {
		notify(alert_msg_copy);
		return;
	}

	void *(*original_func)(GdkDisplay *, GdkNativeWindow, GdkAtom, GdkAtom, GdkAtom, guint32);
	original_func = dlsym(RTLD_NEXT, "gdk_selection_send_notify_for_display");
	(*original_func)(display, requestor, selection, target, property, time_);
}
*/


// Hook "Paste" Operation, gtk based application
/*
GdkWindow *
gdk_selection_owner_get_for_display (GdkDisplay *display, GdkAtom selection)
{
	
	if (disallow_function()) {
		notify(alert_msg_paste);
		//return NULL;
	}

	void *(*original_func)(GdkDisplay *, GdkAtom);
	original_func = dlsym(RTLD_NEXT, "gdk_selection_owner_get_for_display");
	return (*original_func)(display, selection);
}
*/


// Hook Keyboard Event - by GdkKeymap Func
/*
gboolean
gdk_keymap_translate_keyboard_state (GdkKeymap *keymap,
                                     guint hardware_keycode,
                                     GdkModifierType state,
                                     gint group,
                                     guint *keyval,
                                     gint *effective_group,
                                     gint *level,
                                     GdkModifierType *consumed_modifiers)
{
	if (disallow_function()) {
		syslog(LOG_DEBUG, "%ld, %ld, %d, %d, %d, %d\n", hardware_keycode, state, group, *keyval, *effective_group, *level);

		// Ctrl + c
		if(hardware_keycode == (guint)54 && state == (GdkModifierType)20){
			notify(alert_msg_copy);
			return FALSE;
		}

		// Ctrl + v
		if(hardware_keycode == (guint)55 && state == (GdkModifierType)20){
			notify(alert_msg_paste);
			return FALSE;
		}
	}

	gboolean (*original_func)(GdkKeymap *, guint, GdkModifierType, gint, guint *, gint *, gint *, GdkModifierType *);
	original_func = dlsym(RTLD_NEXT, "gdk_keymap_translate_keyboard_state");
	return (*original_func)(keymap, hardware_keycode, state, group, keyval, effective_group, level, consumed_modifiers); 
}
*/


// Hook Keyboard Event by Universal Input Method Func
/*
int uim_press_key(uim_context uc, int key, int state)
{

	if (disallow_function()) {
		sys_log(LOG_DEBUG, "%ld, %ld\n", key, state);

		// Ctrl + c
		if(key == (int)99 && state == (int)2){
			notify(alert_msg_copy);
			return -1;
		}

		// Ctrl + v
		if(key == (int)118 && state == (int)2)
		{
			notify(alert_msg_paste);
			return -1;
		}
	}
	
	//void *handle;
	// Getting Address of Original Function
	//handle = dlopen("im-uim.so", RTLD_NOW);
        //if (!handle) {
        //	fprintf(stderr, "%s\n", dlerror());
        //	exit(EXIT_FAILURE);
        //}

	int (*original_func)(uim_context, int, int);

	original_func = dlsym(RTLD_NEXT, "uim_press_key");
	return (*original_func)(uc, key, state); 

}
*/
