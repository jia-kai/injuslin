/*
 * $File: gtk_addition.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Thu Sep 24 22:50:12 2009
*/
/*
Copyright (C) (2008, 2009) (jiakai) <jia.kai66@gmail.com>

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation; either 
version 2 of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <gdk/gdkkeysyms.h>

#include "gtk_addition.h"
#include "common.h"
using namespace std;


//----------------------Function Declaration----------------------------------
static gboolean cb_input_dialog_entry_key(GtkWidget *entry, GdkEventKey *key, gpointer dialog);
//----------------------------------------------------------------------------

GtkWidget* new_entry_with_label(GtkBox *box, const char *label_txt, const char *default_text)
{
	GtkWidget *hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(box, hbox, FALSE, FALSE, 3);

	GtkWidget *label = gtk_label_new(label_txt);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 3);

	GtkWidget *entry = gtk_entry_new();
	gtk_box_pack_end(GTK_BOX(hbox), entry, TRUE, TRUE, 3);

	if (default_text != NULL)
		gtk_entry_set_text(GTK_ENTRY(entry), default_text);

	return entry;
}

void input_dialog(GtkWindow *parent, const char *msg, string &str)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
			_("Input"), parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL
			);
	GtkWidget *label = gtk_label_new(msg);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, FALSE, 3);
	gtk_widget_show(label);
	
	GtkWidget *entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(entry), "key-press-event",
			G_CALLBACK(cb_input_dialog_entry_key), dialog);
	gtk_entry_set_text(GTK_ENTRY(entry), str.c_str());
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, TRUE, TRUE, 3);
	gtk_widget_show(entry);

	str.clear();

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_OK)
		str.assign(gtk_entry_get_text(GTK_ENTRY(entry)));

	gtk_widget_destroy(dialog);
}

void error_dialog(GtkWindow *parent, const char *msg, GtkMessageType type)
{
	GtkWidget *dialog = gtk_message_dialog_new(
			parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			type, GTK_BUTTONS_OK, "%s",
			msg);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void warning_dialog(GtkWindow *parent, const char *msg, GtkMessageType type)
{
	GtkWidget *dialog = gtk_message_dialog_new(
			parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			type, GTK_BUTTONS_OK, "%s",
			msg);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Warning"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

bool confirm_dialog(GtkWindow *parent, const char *msg, GtkMessageType type)
{
	GtkWidget *dialog = gtk_message_dialog_new(
			parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			type, GTK_BUTTONS_YES_NO, "%s",
			msg);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm"));
	bool ret = (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES);
	gtk_widget_destroy(dialog);
	return ret;
}

void message_dialog(GtkWindow *parent, const char *msg, GtkMessageType type)
{
	GtkWidget *dialog = gtk_message_dialog_new(
			parent, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			type, GTK_BUTTONS_OK, "%s",
			msg);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Message"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

gboolean cb_input_dialog_entry_key(GtkWidget *entry, GdkEventKey *key, gpointer dialog)
{
	if (key->keyval == GDK_Return || key->keyval == GDK_KP_Enter)
		gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
	return FALSE;
}

const char* file_selection_dialog(const char *title, GtkWindow *parent,
		const char *filter_pattern, bool mode_open)
{
	GtkWidget *dialog;
	if (mode_open)
	{
		dialog = gtk_file_chooser_dialog_new(title, parent, GTK_FILE_CHOOSER_ACTION_SAVE, 
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
	}
	else
	{
		dialog = gtk_file_chooser_dialog_new(title, parent, GTK_FILE_CHOOSER_ACTION_SAVE, 
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);
		gtk_file_chooser_set_do_overwrite_confirmation(
				GTK_FILE_CHOOSER(dialog), true);
	}
	if (filter_pattern != NULL)
	{
		GtkFileFilter *filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, filter_pattern);
		gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
	}
	static string path;
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		path.assign(name);
		g_free(name);
	}
	else path.clear();
	gtk_widget_destroy(dialog);
	if (path.empty())
		return NULL;
	return path.c_str();
}

