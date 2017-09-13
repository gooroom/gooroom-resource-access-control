#define _GNU_SOURCE

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <uim/uim.h>

static unsigned char *conf_path="/etc/gooroom/grac.d/hook-clipboard.conf";

static unsigned char *alert_msg_copy="Clipboard(copy) is disallowed!!";
static unsigned char *alert_msg_paste="Clipboard(paste) is disallowed!!";


// Hook "Copy" Operation, gtk based application
/*
void
gdk_selection_send_notify_for_display (GdkDisplay *display,
                                       GdkNativeWindow requestor,
                                       GdkAtom selection,
                                       GdkAtom target,
                                       GdkAtom property,
                                       guint32 time_){

	void *(*original_func)(GdkDisplay *, GdkNativeWindow, GdkAtom, GdkAtom, GdkAtom, guint32);
	FILE *confp;
	int status;

	GtkWidget *dialog;

	original_func = dlsym(RTLD_NEXT, "gdk_selection_send_notify_for_display");

	confp = fopen(conf_path, "r");
	if(confp == NULL)
		(*original_func)(display, requestor, selection, target, property, time_);

	fscanf(confp, "%d", &status);
	fclose(confp);
	
	if(status != 0){
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg_copy);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		
		return;
	}
	
	(*original_func)(display, requestor, selection, target, property, time_);
}
*/


// Hook "Paste" Operation, gtk based application
/*
GdkWindow *
gdk_selection_owner_get_for_display (GdkDisplay *display, GdkAtom selection){
	
	void *(*original_func)(GdkDisplay *, GdkAtom);
	FILE *confp;
	int status;

	GtkWidget *dialog;
                                          
	original_func = dlsym(RTLD_NEXT, "gdk_selection_owner_get_for_display");

	confp = fopen(conf_path, "r");
	if(confp == NULL)
		return (*original_func)(display, selection);

	fscanf(confp, "%d", &status);
	fclose(confp);
	
	if(status != 0){
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg_paste);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		
		return (*original_func)(display, selection);
		//return NULL;
	}
	
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
                                     GdkModifierType *consumed_modifiers){

	gboolean (*original_func)(GdkKeymap *, guint, GdkModifierType, gint, guint *, gint *, gint *, GdkModifierType *);
	FILE *confp, *log_fp;
	int status;

	GtkWidget *dialog;

	// Getting Address of Original Function
	original_func = dlsym(RTLD_NEXT, "gdk_keymap_translate_keyboard_state");
	
	// Check Hooking Configuration
	confp = fopen(conf_path, "r");
	if(confp == NULL)
		return (*original_func)(keymap, hardware_keycode, state, group, keyval, effective_group, level, consumed_modifiers); 

	fscanf(confp, "%d", &status);
	fclose(confp);

	// Hook Operation
	if(status != 0){
		// Key Event Logging
		log_fp = fopen("/home/ultract/gooroom-resource-access-control/hook_clipboard/asdf.txt", "a");
		if(log_fp == NULL)
			return (*original_func)(keymap, hardware_keycode, state, group, keyval, effective_group, level, consumed_modifiers); 

		fprintf(log_fp, "%ld, %ld, %d, %d, %d, %d\n", hardware_keycode, state, group, *keyval, *effective_group, *level);
		fclose(log_fp);

		// Ctrl + c
		if(hardware_keycode == (guint)54 && state == (GdkModifierType)20){
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg_copy);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return;

		// Ctrl + v
		}else if(hardware_keycode == (guint)55 && state == (GdkModifierType)20){
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg_paste);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return;
		}
	}
		
	return (*original_func)(keymap, hardware_keycode, state, group, keyval, effective_group, level, consumed_modifiers); 
}
*/


// Hook Keyboard Event by Universal Input Method Func
/*
int uim_press_key(uim_context uc, int key, int state){
	
	int (*original_func)(uim_context, int, int);
	FILE *confp, *log_fp;
	int status;
	GtkWidget *dialog;
	//void *handle;

	// Getting Address of Original Function
	//handle = dlopen("im-uim.so", RTLD_NOW);
        //if (!handle) {
        //	fprintf(stderr, "%s\n", dlerror());
        //	exit(EXIT_FAILURE);
        //}

	original_func = dlsym(RTLD_NEXT, "uim_press_key");
	
	// Check Hooking Configuration
	confp = fopen(conf_path, "r");
	if(confp == NULL)
		return (*original_func)(uc, key, state); 

	fscanf(confp, "%d", &status);
	fclose(confp);

	// Hook Operation
	if(status != 0){
		// Key Event Logging
		log_fp = fopen("/home/ultract/gooroom-resource-access-control/hook_clipboard/asdf.txt", "a");
		if(log_fp == NULL)
			return (*original_func)(uc, key, state); 

		fprintf(log_fp, "%ld, %ld\n", key, state);
		fclose(log_fp);

		// Ctrl + c
		if(key == (int)99 && state == (int)2){
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg_copy);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return -1;

		// Ctrl + v
		}else if(key == (int)118 && state == (int)2){
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, alert_msg_paste);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			return -1;
		}
	}
		
	return (*original_func)(uc, key, state); 

}
*/
