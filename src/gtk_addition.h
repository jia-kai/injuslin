/*
 * $File: gtk_addition.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Thu Sep 24 22:49:52 2009
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

#ifndef HEADER_GTK_ADDITION
#define HEADER_GTK_ADDITION

#include <gtk/gtk.h>
#include <string>

GtkWidget *new_entry_with_label(GtkBox *box, const char *label, const char *default_text = NULL);
	// @box must be a vbox, and the label and entry group will be packed into @box
void input_dialog(GtkWindow *parent, const char *msg, std::string &str);
	// the initial value should also be stored in @str
void error_dialog(GtkWindow *parent, const char *msg, GtkMessageType type = GTK_MESSAGE_ERROR);
void warning_dialog(GtkWindow *parent, const char *msg, GtkMessageType type = GTK_MESSAGE_WARNING);
void message_dialog(GtkWindow *parent, const char *msg, GtkMessageType type = GTK_MESSAGE_INFO);
bool confirm_dialog(GtkWindow *parent, const char *msg, GtkMessageType type = GTK_MESSAGE_QUESTION);

const char* file_selection_dialog(const char *title, GtkWindow *parent = NULL,
		const char *filter_pattern = NULL, bool mode_open = true); 
	// return NULL if the user canceld

#endif
