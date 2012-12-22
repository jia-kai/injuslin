/*
 * $File: view_results.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Mon Jul 18 14:27:52 2011 +0800
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <time.h>
#include <errno.h>

#include <cassert>
#include <cmath>
#include <vector>
#include <cstring>
#include <map>
#include <cstdio>
using namespace std;

#define NEED_LIST_DIR
#include "common.h"
#include "view_results.h"
#include "structures.h"
#include "gtk_addition.h"


class Page_base
{
public:
	virtual void show(GtkTextBuffer *buffer) = 0;
	virtual ~Page_base() {};
};

class Page_general;
class Page_detail : public Page_base
{
	friend class Page_general;
	typedef vector<Judge_result_problem*> PtrJudge_result_problem_array;

	Page_general *parent;
	string name;
	PtrJudge_result_problem_array result;
	double total_score, total_time;
		// total_time: in seconds

	void export2plain(FILE *fout) const;
	Page_detail(Page_general *parent_, const string &name_, const string &src_dir);
	virtual ~Page_detail();
public:
	void show(GtkTextBuffer *buffer);
};

class Page_general : public Page_base
{
	friend class Page_detail;
	typedef vector<Page_detail*> PtrPage_detail_array;

	PtrPage_detail_array details;
	Str_array prob_name;
	Contest_config ct_conf;
	void sort(int left, int right);
public:
	Page_general(const char *conf_path);
		// global_error_msg will be a non-empty string if and only if an error occurrs.
	virtual ~Page_general();
	void show(GtkTextBuffer *buffer);
	bool export2html(const char *file_path) const;
		// return false on failure, and global_error_msg is set.
	bool export2plain(FILE *fout, const char *contestant) const;
		// return false on failure, and global_error_msg is set.
	const char* get_contest_name() const
		{return ct_conf.contest_name.c_str();}
};

static GtkWindow *main_window;
static GtkTextView *main_window_text_view;
static const char *EXE_STATUS_STR[4], *EXE_STATUS_STR_ENG[4] =
	{"Normal", "Time Out", "Memory Out", "Non-zero Exit Code"};
typedef map<void*, GtkTextTag*> Ptr_TextTag_map;
static Ptr_TextTag_map link_tag_cache;
typedef vector<Str_array> Str_2darray;
//----------------------Function Declaration----------------------------------
static void init_exe_status_str();
static void cb_quit_clicked(GtkWidget *button, gpointer data);
static void cb_export_clicked(GtkWidget *button, gpointer page_general);
static void init_text_buffer_tags(GtkTextBuffer *buffer);
static void print_table(GtkTextBuffer *buffer, GtkTextIter &iter, const Str_2darray &content, Page_base **link = NULL);
	// @content[0] is the head line
static gint get_iter_x(const GtkTextIter *iter);

static void insert_link(GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, Page_base *page);
static void follow_if_link(GtkWidget *text_view, GtkTextBuffer *buffer, GtkTextIter *iter);
static gboolean cb_text_view_key_press_event(GtkWidget *text_view, GdkEventKey *event, gpointer data);
static gboolean cb_text_view_event_after(GtkWidget *text_view, GdkEvent *ev, gpointer data);
static void set_cursor_if_appropriate(GtkTextView *text_view, gint x, gint y);
static gboolean cb_text_view_motion_notify_event(GtkWidget *text_view, GdkEventMotion *event, gpointer data);
static gboolean cb_text_view_visibility_notify_event(GtkWidget *text_view, GdkEventVisibility *event, gpointer data);
static const char* html_special_chars(const char *str);
//----------------------------------------------------------------------------

void view_results(const char *conf_path)
{
	link_tag_cache.clear();
	init_exe_status_str();

// Initialize the pages in the text view
	Page_general page(conf_path);
	if (global_error_msg[0])
	{
		error_dialog(NULL, global_error_msg);
		return;
	}

// Initialize the main window
	GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	main_window = GTK_WINDOW(win);
	char main_window_title[CSTRING_MAX_LEN];
	snprintf(main_window_title, CSTRING_MAX_LEN, "%s - Results [%s]", page.get_contest_name(), VERSION_INF);
	gtk_window_set_title(GTK_WINDOW(win), main_window_title);
	gtk_window_maximize(GTK_WINDOW(win));
	gtk_container_set_border_width(GTK_CONTAINER(win), 10);
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(gtk_main_quit), NULL);

// Initialize the text view
	GtkWidget *view = gtk_text_view_new();
	main_window_text_view = GTK_TEXT_VIEW(view);

	g_signal_connect(G_OBJECT(view), "key-press-event",
			G_CALLBACK(cb_text_view_key_press_event), NULL);
	g_signal_connect(G_OBJECT(view), "event-after",
			G_CALLBACK(cb_text_view_event_after), NULL);
	g_signal_connect(G_OBJECT(view), "motion-notify-event",
			G_CALLBACK(cb_text_view_motion_notify_event), NULL);
	g_signal_connect(G_OBJECT(view), "visibility-notify-event",
			G_CALLBACK(cb_text_view_visibility_notify_event), NULL);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
	gtk_text_view_set_tabs(GTK_TEXT_VIEW(view), pango_tab_array_new(0, TRUE));
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	init_text_buffer_tags(buffer);
	GtkWidget *view_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(view_sw),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(view_sw), view);
	page.show(buffer);

// Create necessary boxes
	GtkWidget *vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), view_sw, TRUE, TRUE, 3);
	GtkWidget *hbox = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

// Initialize the buttons
	GtkWidget *button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	g_signal_connect(G_OBJECT(button), "clicked", 
			G_CALLBACK(cb_quit_clicked), NULL);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 3);
	button = gtk_button_new_with_mnemonic(_("_Export as HTML"));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_export_clicked), &page);
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 3);

	gtk_widget_show_all(win);
	gtk_main();

}

void export_results2html(const char *conf_path, const char *file_path)
{
	Page_general page(conf_path);
	if (global_error_msg[0])
	{
		fprintf(stderr, "%s\n", global_error_msg);
		return;
	}
	init_exe_status_str();
	assert(fchdir(fd_orig_work_dir) == 0);
	if (!page.export2html(file_path))
		fprintf(stderr, "%s\n", global_error_msg);
	assert(fchdir(fd_config_work_dir) == 0);
}

void export_results2stdout(const char *conf_path, const char *contestant)
{
	Page_general page(conf_path);
	if (global_error_msg[0])
	{
		fprintf(stderr, "%s\n", global_error_msg);
		return;
	}
	init_exe_status_str();
	if (!page.export2plain(stdout, contestant))
		fprintf(stderr, "%s\n", global_error_msg);
}

void init_exe_status_str()
{
	EXE_STATUS_STR[0] = _("Normal");
	EXE_STATUS_STR[1] = _("Time Out");
	EXE_STATUS_STR[2] = _("Memory Out");
	EXE_STATUS_STR[3] = _("Non-zero Exit Code");
}

void cb_quit_clicked(GtkWidget *button, gpointer data)
{
	pango_tab_array_free(gtk_text_view_get_tabs(main_window_text_view));
	gtk_widget_destroy(GTK_WIDGET(main_window));
}

void init_text_buffer_tags(GtkTextBuffer *buffer)
{
#define NS gtk_text_buffer_create_tag(buffer,
#define NE , NULL)
	NS "page_title", "weight", PANGO_WEIGHT_BOLD, "scale", PANGO_SCALE_XX_LARGE NE;
	NS "center", "justification", GTK_JUSTIFY_CENTER NE;
	NS "right", "justification", GTK_JUSTIFY_RIGHT NE;
	NS "table_head", "foreground", "red" NE;
	NS "prob_name", "weight", PANGO_WEIGHT_BOLD, "scale", PANGO_SCALE_X_LARGE NE;
	NS "compile_error", "foreground", "magenta" NE;
#undef NS
#undef NE
}

void cb_export_clicked(GtkWidget *button, gpointer page_general0)
{
	Page_general *pg = static_cast<Page_general*>(page_general0);
	const char *path = file_selection_dialog(
			_("Please select the output file"), main_window, "*.html", false);
	if (path != NULL)
		if (!pg->export2html(path))
			error_dialog(main_window, global_error_msg);
}

//--------------------- Hypertext Fuctions Start ------------------------------------------
void insert_link(GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, Page_base *page)
{
	Ptr_TextTag_map::iterator it = link_tag_cache.find(page);
	GtkTextTag *tag;
	if (it != link_tag_cache.end())
		tag = it->second;
	else
	{
		tag = gtk_text_buffer_create_tag(buffer, NULL, 
				"foreground", "blue", 
				"underline", PANGO_UNDERLINE_SINGLE, 
				NULL);
		g_object_set_data(G_OBJECT(tag), "page", page);
	}
	gtk_text_buffer_insert_with_tags(buffer, iter, text, -1, tag, NULL);
}

void follow_if_link(GtkWidget *text_view, GtkTextBuffer *buffer, GtkTextIter *iter)
{
	GSList *tags = NULL;

	tags = gtk_text_iter_get_tags(iter);
	for (GSList *tagp = tags; tagp != NULL; tagp = tagp->next)
	{
		GtkTextTag *tag = static_cast<GtkTextTag*>(tagp->data);
		Page_base *page = static_cast<Page_base*>(g_object_get_data(G_OBJECT(tag), "page"));
		if (page != NULL)
			page->show(buffer);
	}

	if (tags) 
		g_slist_free (tags);
}

// activate by pressing ENTER
gboolean cb_text_view_key_press_event(GtkWidget *text_view, GdkEventKey *event, gpointer data)
{
	if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
	{
		GtkTextIter iter;
		GtkTextBuffer *buffer =
			gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
		gtk_text_buffer_get_iter_at_mark(buffer, &iter, 
				gtk_text_buffer_get_insert(buffer));
		follow_if_link(text_view, buffer, &iter);
	}
	return FALSE;
}

// activate by a click
gboolean cb_text_view_event_after(GtkWidget *text_view, GdkEvent *ev, gpointer data)
{
	if (ev->type != GDK_BUTTON_RELEASE)
		return FALSE;

	GdkEventButton *event = (GdkEventButton*)ev;

	if (event->button != 1)
		return FALSE;

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

	gint x, y;
	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
			GTK_TEXT_WINDOW_WIDGET,
			(gint)(event->x), (gint)(event->y), &x, &y);

	GtkTextIter iter;
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW (text_view), &iter, x, y);
	follow_if_link (text_view, buffer, &iter);

	return FALSE;
}

void set_cursor_if_appropriate(GtkTextView *text_view, gint x, gint y)
{
	static gboolean hovering_over_link = FALSE;
	static GdkCursor *hand_cursor = NULL, *regular_cursor = NULL;

	if (hand_cursor == NULL)
	{
		hand_cursor = gdk_cursor_new(GDK_HAND2);
		regular_cursor = gdk_cursor_new(GDK_XTERM);
	}

	GtkTextIter iter;
	gtk_text_view_get_iter_at_location(text_view, &iter, x, y);

	GSList *tags = gtk_text_iter_get_tags(&iter);

	gboolean hovering = FALSE;
	for (GSList *tagp = tags; tagp != NULL; tagp = tagp->next)
	{
		GtkTextTag *tag = static_cast<GtkTextTag*>(tagp->data);
		Page_base *page = static_cast<Page_base*>(g_object_get_data(G_OBJECT(tag), "page"));
		if (page != NULL) 
		{
			hovering = TRUE;
			break;
		}
	}

	if (hovering != hovering_over_link)
	{
		hovering_over_link = hovering;

		if (hovering_over_link)
			gdk_window_set_cursor(gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT), hand_cursor);
		else
			gdk_window_set_cursor(gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT), regular_cursor);
	}

	if (tags) 
		g_slist_free (tags);
}

gboolean cb_text_view_motion_notify_event(GtkWidget *text_view, GdkEventMotion *event, gpointer data)
{
	gint x, y;

	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
			GTK_TEXT_WINDOW_WIDGET,
			(gint)(event->x), (gint)(event->y), &x, &y);

	set_cursor_if_appropriate(GTK_TEXT_VIEW(text_view), x, y);

	gdk_window_get_pointer(text_view->window, NULL, NULL, NULL);
	return FALSE;
}

gboolean cb_text_view_visibility_notify_event(GtkWidget *text_view, GdkEventVisibility *event, gpointer data)
{
	gint wx, wy, bx, by;

	gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);

	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
			GTK_TEXT_WINDOW_WIDGET,
			wx, wy, &bx, &by);

	set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), bx, by);

	return FALSE;
}
//--------------------- Hypertext Fuctions End --------------------------------------------

Page_general::Page_general(const char *conf_path)
{
	global_error_msg[0] = 0;

	Conf conf_file(conf_path);
	if (!ct_conf.init(conf_file))
	{
		string tmp(global_error_msg);
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("%s : %s : %d: \n%s\nDetails:\n%s"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__, _("\
Error while initializing the contest configuration.\n\
Maybe the configuration file is broken."), tmp.c_str());
		return;
	}

	for (Problem_config_array::const_iterator iter = ct_conf.problems.begin(); 
			iter != ct_conf.problems.end(); iter ++)
	{
		prob_name.push_back(iter->name);
	}

	Str_array cta_name;
	if (!list_dir_in_dir(ct_conf.source_dir.c_str(), cta_name))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Failed to get the contestants:\n %s\n (At %s : %s : %d)"),
				strerror(errno), __FILE__, __PRETTY_FUNCTION__, __LINE__);
		return;
	}
	if (cta_name.size() == 0)
	{
		strcpy(global_error_msg, _("There is no contestant."));
		return;
	}

	for (Str_array::iterator iter = cta_name.begin(); iter != cta_name.end(); iter ++)
		details.push_back(new Page_detail(this, *iter, ct_conf.source_dir));

	sort(0, details.size() - 1);
}

void Page_general::sort(int left, int right)
{
	if (left >= right)
		return;
	int i = left, j = right;
	Page_detail *mid = details[(left + right) >> 1];

#define LESS(_a_, _b_) (((_a_)->total_score > (_b_)->total_score) || \
		((_a_)->total_score == (_b_)->total_score && (_a_)->total_time < (_b_)->total_time))
	while (i <= j)
	{
		while (LESS(details[i], mid))
			i ++;
		while (LESS(mid, details[j]))
			j --;
		if (i <= j)
		{
			Page_detail *tmp = details[i];
			details[i] = details[j];
			details[j] = tmp;
			i ++;
			j --;
		}
	}
#undef LESS

	sort(left, j);
	sort(i, right);
}

Page_general::~Page_general()
{
	for (PtrPage_detail_array::iterator iter = details.begin(); iter != details.end(); iter ++)
		delete *iter;
}

Page_detail::Page_detail(Page_general *parent_, const string &name_, const string &src_dir) :
	parent(parent_), name(name_), total_score(0), total_time(0)
{
	result.resize(parent->prob_name.size(), NULL);
	Conf conf((src_dir + name + "/result.cfg").c_str());
	string version;
	if (!conf.read("general", "VERSION", version) || version != JUDGE_RESULT_CONF_VERSION)
		return;

	for (Str_array::size_type i = 0; i < result.size(); i ++)
	{
		string str;
		if (!conf.read("result", parent->prob_name[i].c_str(), str))
			continue;
		Judge_result_problem *tmp = new Judge_result_problem;
		if (!tmp->init(str))
		{
			delete tmp;
			continue;
		}
		result[i] = tmp;
		ITER_VECTOR(tmp->points, j)
		{
			total_score += j->score;
			if (j->score > 0)
				total_time += j->time;
		}
	}
	total_time *= 1e-6;
}

Page_detail::~Page_detail()
{
	for (PtrJudge_result_problem_array::iterator iter = result.begin(); iter != result.end(); iter ++)
		if (*iter != NULL)
			delete *iter;
}

#define SHOW_INIT \
	static char msg[CSTRING_MAX_LEN]; \
	gtk_text_buffer_set_text(buffer, "", 0); \
	GtkTextIter iter, iter_prev; \
	gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0); \
	gint iter_offset

#define ITNS gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, 
#define ITNE , NULL)

#define IS gtk_text_buffer_insert(buffer, &iter,
#define IE )

#define ATN_INIT iter_offset = gtk_text_iter_get_offset(&iter)
#define ATNS \
	gtk_text_buffer_get_iter_at_offset(buffer, &iter_prev, iter_offset); \
	gtk_text_buffer_apply_tag_by_name(buffer,
#define ATNE , &iter_prev, &iter)

void Page_general::show(GtkTextBuffer *buffer)
{
	SHOW_INIT;

	ITNS _("Ranked Grades\n"), -1, "page_title", "center" ITNE;

	Page_base **link = new Page_base*[details.size() + 1];
	link[0] = NULL;

	Str_2darray table;
	table.resize(details.size() + 1);
	table[0].resize(4);
	table[0][0] = _("Rank");
	table[0][1] = _("Name");
	table[0][2] = _("Total Score");
	table[0][3] = _("Total Time [sec]");
	int rank = 1;
	for (Str_2darray::iterator iter_row = table.begin() + 1; iter_row != table.end(); iter_row ++)
	{
		iter_row->resize(4);
		static char tmp[CSTRING_MAX_LEN];
		snprintf(tmp, CSTRING_MAX_LEN, "%d", rank ++);
		iter_row->at(0).assign(tmp);

		Page_detail *d = details[rank - 2];
		link[rank - 1] = d;
		iter_row->at(1).assign(d->name);
		snprintf(tmp, CSTRING_MAX_LEN, "%.3lf", d->total_score);
		iter_row->at(2).assign(tmp);
		snprintf(tmp, CSTRING_MAX_LEN, "%.3lf", d->total_time);
		iter_row->at(3).assign(tmp);
	}

	ATN_INIT;
	print_table(buffer, iter, table, link);
	ATNS "center" ATNE;
}

void Page_detail::show(GtkTextBuffer *buffer)
{
	SHOW_INIT;

	snprintf(msg, CSTRING_MAX_LEN, _("Details for contestant '%s'\n"), name.c_str());
	ITNS msg, -1, "page_title", "center" ITNE;

	for (Str_array::size_type i = 0; i < parent->prob_name.size(); i ++)
	{
		snprintf(msg, CSTRING_MAX_LEN, _("Problem: %s\n"), parent->prob_name[i].c_str());
		ITNS msg, -1, "prob_name" ITNE;
		if (result[i] == NULL)
			IS _("Not Judged.\n"), -1 IE;
		else
		{
			Judge_result_problem *jr_prob = result[i];
			if (!jr_prob->source_found)
				IS _("Source file is not found.\n"), -1 IE;
			else if (!jr_prob->compile_successful)
			{
				IS _("Failed to compile.\nDetails:\n"), -1 IE;
				ITNS jr_prob->compiler_output.c_str(), -1, "compile_error" ITNE;
			}else
			{
				Str_2darray table;
				ATN_INIT;
				table.resize(jr_prob->points.size() + 1);
				table[0].resize(5);
				table[0][0] = _("Point");
				table[0][1] = _("Execution Status");
				table[0][2] = _("Score");
				table[0][3] = _("Time [sec]");
				table[0][4] = _("Memory [kb]");
				int count = 0;
				for (Str_2darray::iterator iter_row = table.begin() + 1; iter_row != table.end(); iter_row ++)
				{
					Judge_result_point &jr_point = jr_prob->points.at(count ++);
					snprintf(msg, CSTRING_MAX_LEN, "%d", count);
					iter_row->resize(5);
					iter_row->at(0).assign(msg);
					iter_row->at(1).assign(EXE_STATUS_STR[(int)jr_point.exe_status]);
					snprintf(msg, CSTRING_MAX_LEN, "%.3lf", jr_point.score);
					iter_row->at(2).assign(msg);
					snprintf(msg, CSTRING_MAX_LEN, "%.3lf", jr_point.time * 1e-6);
					iter_row->at(3).assign(msg);
					snprintf(msg, CSTRING_MAX_LEN, "%ld", jr_point.memory);
					iter_row->at(4).assign(msg);
				}

				print_table(buffer, iter, table); 
				ATNS "center" ATNE;
			}
		}
		IS "\n", -1 IE;
	}

	ATN_INIT;
	insert_link(buffer, &iter, _("Go Back"), parent);
	ATNS "right" ATNE;

}

void print_table(GtkTextBuffer *buffer, GtkTextIter &iter, const Str_2darray &content, Page_base **link)
{
// The implement of the function is ugly and inefficient...
// How to draw a table in a GtkTextView?
	static const int PADDING = 10;

	gint iter_orig_offset = gtk_text_iter_get_offset(&iter), last_x = get_iter_x(&iter);
	gint col_width = 0;
	for (Str_2darray::const_iterator iter_row = content.begin(); iter_row != content.end(); iter_row ++)
	{
		for (Str_array::const_iterator iter_col = iter_row->begin(); iter_col != iter_row->end(); iter_col ++)
		{
			IS iter_col->c_str(), -1 IE;
			gint tmp = get_iter_x(&iter) - last_x;
			if (tmp > col_width)
				col_width = tmp;
			GtkTextIter iter_orig;
			gtk_text_buffer_get_iter_at_offset(buffer, &iter_orig, iter_orig_offset);
			gtk_text_buffer_delete(buffer, &iter_orig, &iter);
		}
	}
	col_width += PADDING;

	int tabs_size = content[0].size() + 2;
	PangoTabArray* tabs = gtk_text_view_get_tabs(main_window_text_view);
	pango_tab_array_resize(tabs, tabs_size);
	for (int i = 0; i < tabs_size; i ++)
		pango_tab_array_set_tab(tabs, i, PANGO_TAB_LEFT, col_width * i);
	gtk_text_view_set_tabs(main_window_text_view, tabs);

	for (Str_array::size_type i = 0; i < content[0].size(); i ++)
	{
		ITNS content[0][i].c_str(), -1, "table_head", "center" ITNE;
		IS "\t", -1 IE;
	}

	IS "\n", -1 IE;

	for (Str_2darray::size_type i = 1; i < content.size(); i ++)
	{
		for (Str_array::size_type j = 0; j < content[i].size(); j ++)
		{
			if (j == 1)
			{
				if (link != NULL)
					insert_link(buffer, &iter, content[i][1].c_str(), link[i]);
				else IS content[i][1].c_str(), -1 IE;
			} else
				IS content[i][j].c_str(), -1 IE;
			IS "\t", -1 IE;
		}

		IS "\n", -1 IE;
	}

}

#undef SHOW_INIT
#undef ITNS
#undef ITNE
#undef IS
#undef IE
#undef ATN_INIT
#undef ATNS
#undef ATNE

gint get_iter_x(const GtkTextIter *iter)
{
	GdkRectangle loc;
	gtk_text_view_get_iter_location(main_window_text_view, iter, &loc);
	return loc.x;
}

bool Page_general::export2html(const char *file_path) const
{
	static const char *COLOR[] = 
		{"green", "#FF0F05", "#FF0F05", "navy"};
	FILE *fout = fopen(file_path, "w");
	static char msg[CSTRING_MAX_LEN];
	if (fout == NULL)
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("%s : %s : %d :  Failed to open file ('%s').\nDetails:\n%s\n"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__, file_path, strerror(errno));
		return false;
	}
	fprintf(fout, "<html>\n");
	fprintf(fout, "<head>\n");
	fprintf(fout, "<meta content=\"text/html; charset=UTF-8\" http-equiv=\"Content-Type\" />\n");
	fprintf(fout, _("<title> Judge Results for Contest \"%s\"</title>\n"), 
			html_special_chars(ct_conf.contest_name.c_str()));
	fprintf(fout, "</head>\n");
	fprintf(fout, 
	"<style> <!-- \
	body \
	{ \
		background-color:	#EBE8FF; \
	} \
	body, table, div, td \
	{ \
		font-size:			14px; \
	} \
	th \
	{ \
		font-weight:	bold; \
		height:			21px; \
		font-size:		15px; \
	} \
	th, td \
	{ \
		text-align: center; \
	} \
	tr.odd \
	{ \
		background-color:	#E0E0E0; \
	} \
	tr.even \
	{ \
		background-color:	#C5C5C5; \
	} \
	a:link, a:visited \
	{ \
		text-decoration:	none; \
		color:				#0000FF; \
	} \
	a:hover \
	{ \
		text-decoration:	underline; \
		color:				#F000FF; \
	} \
	tr:hover \
	{ \
		background-color:	#F8F8F8; \
	} \
	a.none:hover \
	{ \
		text-decoration:	none; \
		color:				black; \
	} \
	--> </style>");

	static const char *TR_CLASS[2] = {"class=\"odd\"", "class=\"even\""};
	int tr_class = 0;
	fprintf(fout, "<body><center>\n");

	// Ranked Grades start
	fprintf(fout, "<B><h1><a name=\"top\" class=\"none\">%s</a></h1></B>\n", _("Ranked Grades"));
	fprintf(fout, "<table>\n");
	fprintf(fout, "<tr %s><th>%s</th><th>%s</th><th>%s</th><th>%s</th>\n", TR_CLASS[tr_class ^= 1],
			_("Rank"), _("Name"), _("Total Score"), _("Total Time [sec]"));
	ITER_VECTOR(prob_name, i)
		fprintf(fout, "<th>%s</th>", html_special_chars(i->c_str()));
	fprintf(fout, "</tr>\n");

	ITER_VECTOR_IDX(details, i)
	{
		fprintf(fout, "<tr %s><td>%d</td><td><a href=\"#%d\">%s</a></td>"
				"<td>%0.3lf</td><td>%0.3lf</td>\n", 
				TR_CLASS[tr_class ^= 1],
				(int)i + 1, (int)i, html_special_chars(details[i]->name.c_str()), details[i]->total_score, details[i]->total_time);
		ITER_VECTOR_IDX(prob_name, j)
		{
			double score = 0, time = 0;
			Judge_result_problem *p = details[i]->result[j];
			if (p && p->source_found && p->compile_successful)
			{
				ITER_VECTOR(p->points, k)
				{
					score += k->score;
					if (k->score > 0)
						time += k->time * 1e-6;
				}
			}
			fprintf(fout, "<td><a href=\"#%d:%d\">%.0lf (%.1lf s)</a></td>", (int)i, (int)j, score, time);
		}
		fprintf(fout, "</tr>\n");
	}
	fprintf(fout, "</table>\n");
	// Ranked Grades end

	// Details start
	for (Str_array::size_type i = 0; i < details.size(); i ++)
	{
		fprintf(fout, "<hr />\n");
		fprintf(fout, _("<a name=\"%d\" class=\"none\"><B><h3>Details for Contestant \"%s\"</h3></B></a>\n"),
				(int)i, html_special_chars(details[i]->name.c_str()));
		fprintf(fout, "<table>\n");
		fprintf(fout, "<tr %s><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th>",
				TR_CLASS[tr_class ^= 1],
				_("Problem"), _("Point"), _("Execution Status"), _("Score"), _("Time [sec]"), _("Memory [kb]"));
		for (Str_array::size_type j = 0; j < prob_name.size(); j ++)
		{
			if (details[i]->result[j] == NULL)
			{
				fprintf(fout, "<tr %s><th>%s</th><td colspan=5>%s</td></tr>\n", TR_CLASS[tr_class ^= 1],
						html_special_chars(prob_name[j].c_str()), _("Not Judged."));
				continue;
			}
			Judge_result_problem *jr_prob = details[i]->result[j];
			if (!(jr_prob->source_found))
				fprintf(fout, "<tr %s><th><a name='%d:%d' class='none'>%s</a></th><td colspan=5>%s</td></tr>\n", TR_CLASS[tr_class ^= 1],
						(int)i, (int)j,
						html_special_chars(prob_name[j].c_str()), _("Source file is not found."));
			else if (!(jr_prob->compile_successful))
				fprintf(fout, "<tr %s><th><a name='%d:%d'>%s</a></th><td colspan=5><pre>%s</pre>\
						<font color=\"navy\"><pre>%s</pre></font></td></tr>\n",
						TR_CLASS[tr_class ^= 1], (int)i, (int)j, html_special_chars(prob_name[j].c_str()),
						_("Failed to compile.\nDetails:\n"), jr_prob->compiler_output.c_str());
			else 
			{
				fprintf(fout, "<tr %s><th rowspan=%d>%s</th>\n", TR_CLASS[tr_class ^= 1],
						(int)jr_prob->points.size(), html_special_chars(prob_name[j].c_str()));
				for (Str_array::size_type k = 0; k < jr_prob->points.size(); k ++)
				{
					if (k > 0)
					{
						fprintf(fout, "<tr %s>", TR_CLASS[tr_class ^= 1]);
						fprintf(fout, "<td>%d</td>", int(k + 1));
					}
					else
					{
						fprintf(fout, "<td><a name=\"%d:%d\" class=\"none\">%d</a></td>",
							int(i), int(j), int(k + 1));
					}
					Judge_result_point &jr_point = jr_prob->points[k];
					int ch = (int)jr_point.exe_status;
					if (jr_point.score > 0)
						fprintf(fout, "<td><font color=\"%s\">%s</font></td><td>\
								<font color=\"green\">%0.3f</font></td><td>%0.3f</td><td>%lu</td>",
								COLOR[ch], EXE_STATUS_STR[ch], jr_point.score, jr_point.time * 1e-6, jr_point.memory);
					else fprintf(fout, "<td><font color=\"%s\">%s</font></td><td>%0.3f</td><td>%0.3f</td><td>%lu</td>",
							COLOR[ch], EXE_STATUS_STR[ch], jr_point.score, jr_point.time * 1e-6, jr_point.memory);
					fprintf(fout, "</tr>\n");
				}
			}
		}
		fprintf(fout, "</table><br /><p align=\"right\"><a href=\"#top\">%s</a></p>\n", _("Top"));
	}
	// Details start

	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	if (tmp == NULL)
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, "localtime: %s", strerror(errno));
		return false;
	}
	char timestr[CSTRING_MAX_LEN];
	strftime(timestr, CSTRING_MAX_LEN, _("%H:%M:%S ,%a %b %d, %Y"), tmp);
	fprintf(fout, "</center><br /><br /><hr /><p align=\"right\">\
			<small>created by <a href=\"http://injuslin.sourceforge.net\" target=\"_blank\" style=\"color:black\">\
			%s</a><br />%s</small></p></body>\n</html>\n", VERSION_INF, timestr);
	fclose(fout);
	return true;
}

bool Page_general::export2plain(FILE *fout, const char *contestant) const
{
	if (contestant != NULL)
	{
		for (PtrPage_detail_array::const_iterator iter = details.begin(); iter != details.end(); iter ++)
		{
			if ((*iter)->name == contestant)
			{
				(*iter)->export2plain(fout);
				return true;
			}
		}
		strcpy(global_error_msg, _("Failed to export: No such contestant."));
		return false;
	}
	for (PtrPage_detail_array::const_iterator iter = details.begin(); iter != details.end(); iter ++)
		(*iter)->export2plain(fout);
	return true;
}

void Page_detail::export2plain(FILE *fout) const
{
// FIX ME:
//  i18n is not supported, because it's hard to deal with wide characters.
	fprintf(fout, "Contestant: %s\n", name.c_str());
	fprintf(fout, " Total score: %.3lf Total time: %.3lf [sec]\n", total_score, total_time);
	for (Str_array::size_type i = 0; i < parent->prob_name.size(); i ++)
	{
		fprintf(fout, "  Problem: %s\n", parent->prob_name[i].c_str());
		if (result[i] == NULL)
		{
			fprintf(fout, "   %s\n\n", "Not judged.");
			continue;
		}
		if (!result[i]->source_found)
		{
			fprintf(fout, "   %s\n\n", "Source file is not found.");
			continue;
		}
		if (!result[i]->compile_successful)
		{
			fprintf(fout, "   %s%s\n\n", "Failed to compile.\nDetails:\n", result[i]->compiler_output.c_str());
			continue;
		}
		fprintf(fout, "    %-8s %-20s %-13s %-13s %-13s\n",
				"Point", "Execution Status", "Score", "Time [sec]", "Memory [kb]");
		for (Str_array::size_type j = 0; j < result[i]->points.size(); j ++)
		{
			Judge_result_point &cur = result[i]->points[j];
			fprintf(fout, "    %-8d %-20s %-13.3lf %-13.3lf %-13lu\n",
					(int)j + 1, EXE_STATUS_STR_ENG[(int)cur.exe_status], cur.score, cur.time * 1e-6, cur.memory);
		}
		fprintf(fout, "\n");
	}
	fprintf(fout, "\n");
}

const char* html_special_chars(const char *str)
{
/*
	& -> &amp;
	< -> &lt;
	> -> &rt;
	" -> &quot;
	' -> &#039;
	  -> &nbsp;
*/
	static string val;
	val.clear();
	for (const char *ptr = str; *ptr; ptr ++)
		if (*ptr == '&')
			val.append("&amp;");
		else if (*ptr == '<')
			val.append("&lt;");
		else if (*ptr == '>')
			val.append("&gt;");
		else if (*ptr == '"')
			val.append("&quot;");
		else if (*ptr == '\'')
			val.append("&#039;");
		else if (*ptr == ' ')
			val.append("&nbsp;");
		else val.append(1, *ptr);
	return val.c_str();
}

