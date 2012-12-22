/*
 * $File: configure_contest.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Jul 06 16:39:17 2011 +0800
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
#include <unistd.h>
#include <errno.h>

#include <cstdio>
#include <cassert>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <set>
using namespace std;

#define NEED_LIST_DIR
#include "common.h"
#include "structures.h"
#include "gtk_addition.h"
#include "auto_add_points.h"
#include "structures.h"

#define DEFAULT_DATA_DIR	"data"

struct Cb_modify_contest_config_data
{
	Contest_config *ptr_ct_conf;
	GtkWindow *main_win;
	GtkWidget *entry_name, *entry_srcdir, *combo_box_ext, *entry_command;
	int combo_box_prev_select;
	Str_array compiler_ext, compiler_command;
	Str_map *ptr_orig_map;
};

struct Prob_info_frame_data
{
	GtkWidget *entry_name, *entry_input, *entry_output, *entry_source, *entry_cmp_func;
	GtkWidget *check_button_user_cmp;
};

struct Point_info_frame_data
{
	GtkWidget *sb_time; // sb: spin button
	GtkWidget *entry_rl_as, *entry_rl_data, *entry_rl_stack, // rl: rlimit
			  *entry_score,
			  *entry_input, *entry_output;
};

struct Cb_selection_changed_data
{
	Contest_config *ptr_ct_conf;
	Prob_info_frame_data *ptr_prif;
	Point_info_frame_data *ptr_poif;
	int nbp_none, nbp_problem, nbp_point;
	int *selection;
	GtkWidget *button_delete, *button_add, *button_auto_add, *notebook_info;
	void update_ct_conf();
	void update_info_frame();
};

struct Cb_add_delete_data
{
	GtkWidget *tree_view;
	Contest_config *ptr_ct_conf;
	int *selection;
};

struct Cb_quit_clicked_data
{
	GtkWidget *tree_view;
	Contest_config *ptr_ct_conf;
};

static GtkWindow *main_window;
//----------------------Function Declaration----------------------------------
static GtkWidget* create_view_and_model();
static void tree_view_add_problem(GtkWidget *view, const char *str);
static void tree_view_add_points(GtkWidget *view, int prob_num, int pcount);
static void tree_view_remove_problem(GtkWidget *view, int prob_num);
static void tree_view_remove_points(GtkWidget *view, int prob_num, int pcount);
	// remove the last @pcount points of the  problem
static void simulate_tree_view_change(GtkWidget *view);

static GtkWidget* create_prob_info_frame(Prob_info_frame_data &data, Cb_add_delete_data *ptr_add_del_data);
static gboolean cb_prob_name_entry_changed(GtkWidget *entry, GdkEventKey *key, gpointer data);
static void cb_prif_togglebutton_toggled(GtkToggleButton *button, gpointer entry);

static GtkWidget* create_point_info_frame(Point_info_frame_data &data);

static void cb_add_test_points_clicked(GtkButton *button, gpointer data);
static void cb_auto_add_test_points_clicked(GtkButton *button, gpointer data);
static void cb_auto_configure_clicked(GtkButton *button, gpointer data);
static void auto_add_test_points_local(GtkWidget *tree_view, int prob_num, Problem_config &pconf);
static void cb_delete_clicked(GtkButton *button, gpointer data);
static void cb_add_problem_clicked(GtkButton *button, gpointer data);
static void cb_selection_changed(GtkTreeSelection *selection, gpointer data);
static void cb_general_options_clicked(GtkButton *button, gpointer conf);
static void cb_quit_clicked(GtkButton *button, gpointer data);

static void modify_init(Contest_config &conf);
static void cb_modify_contest_config_ok(GtkButton *button, gpointer data);
static void cb_modify_contest_config_destroy(GtkObject *obj, gpointer data);
static void cb_modify_contest_config_combo_box_changed(GtkComboBox *widget, gpointer data);
static void cb_modify_contest_config_add_clicked(GtkButton *button, gpointer data);
static void cb_modify_contest_config_remove_clicked(GtkButton *button, gpointer data);

static void set_default_conf(Conf &conf);
static void set_main_window_title(const char *contest_name);
//----------------------------------------------------------------------------


void configure_contest(const char *conf_path)
{
	Conf conf_file(conf_path);
	if (conf_file.empty())
		set_default_conf(conf_file);
	Contest_config ct_conf;
	if (!ct_conf.init(conf_file))
	{
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, 
				GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, _("%s\nDetails:\n%s"),
				_("\
Error while initializing the contest configuration.\n\
Maybe the configuration file is broken."), global_error_msg);
		gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
		g_signal_connect_swapped(G_OBJECT(dialog), "response", 
				G_CALLBACK(gtk_widget_destroy), dialog);
		g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_main_quit), NULL);
		gtk_widget_show(dialog);
		gtk_main();
		return;
	}

// Initialize the main window
	GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	main_window = GTK_WINDOW(win);
	set_main_window_title(ct_conf.contest_name.c_str());
	gtk_container_set_border_width(GTK_CONTAINER(win), 10);
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(gtk_main_quit), NULL);


// Create and set boxes
	GtkWidget *vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	GtkWidget *hbox_top = gtk_hbox_new(FALSE, 3), 
			  *hbox_bottom = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox_top, TRUE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 3);

// Some static data structures.  
// Note: The initialization is to be completed later.
	static Cb_selection_changed_data cb_sc_data;
	cb_sc_data.ptr_ct_conf = &ct_conf;

	static int tree_view_selection[2];
	tree_view_selection[0] = tree_view_selection[1] = -1;
	cb_sc_data.selection = tree_view_selection;

	static Prob_info_frame_data prif_data;
	cb_sc_data.ptr_prif = &prif_data;

	static Point_info_frame_data poif_data;
	cb_sc_data.ptr_poif = &poif_data;

	static Cb_add_delete_data cb_add_del_data;
	cb_add_del_data.ptr_ct_conf = &ct_conf;
	cb_add_del_data.selection = tree_view_selection;

	static Cb_quit_clicked_data cb_quit_clicked_data;
	cb_quit_clicked_data.ptr_ct_conf = &ct_conf;

// First, create the tree_view structure
	GtkWidget *tree_view = create_view_and_model();
	cb_add_del_data.tree_view = tree_view;
	cb_quit_clicked_data.tree_view = tree_view;

// Create the notebook to show information
//   about the current problem or point.
	GtkWidget *info_nb = gtk_notebook_new();
	GtkWidget *frame_prob = create_prob_info_frame(prif_data, &cb_add_del_data),
			  *frame_point = create_point_info_frame(poif_data);
	gtk_notebook_append_page(GTK_NOTEBOOK(info_nb), 
			gtk_frame_new(_("NONE")), NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(info_nb), frame_prob, NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(info_nb), frame_point, NULL);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(info_nb), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(info_nb), FALSE);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(info_nb), TRUE);
	gtk_box_pack_end(GTK_BOX(hbox_top), info_nb, TRUE, TRUE, 3);
	cb_sc_data.nbp_none = 0;
	cb_sc_data.nbp_problem = 1;
	cb_sc_data.nbp_point = 2;
	cb_sc_data.notebook_info = info_nb;

// Initialize the buttons.
	GtkWidget *button = gtk_button_new_with_mnemonic(_("General _Options"));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_general_options_clicked), &ct_conf);
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);

	button = gtk_button_new_with_mnemonic(_("Auto. _Configure"));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_auto_configure_clicked), &cb_add_del_data);
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);

	button = gtk_button_new_with_mnemonic(_("_Add Problem"));
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_add_problem_clicked), &cb_add_del_data);

	hbox_bottom = gtk_hbox_new(TRUE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox_bottom, FALSE, FALSE, 3);

	button = gtk_button_new_with_mnemonic(_("_Delete Problem"));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_delete_clicked), &cb_add_del_data);
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);
	cb_sc_data.button_delete = button;
	gtk_widget_set_sensitive(button, FALSE);

	button = gtk_button_new_with_mnemonic(_("Add _Test Points"));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_add_test_points_clicked), &cb_add_del_data);
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);
	cb_sc_data.button_add = button;
	gtk_widget_set_sensitive(button, FALSE);

	button = gtk_button_new_with_mnemonic(_("Auto. Add Test _Points"));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_auto_add_test_points_clicked), &cb_add_del_data);
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);
	cb_sc_data.button_auto_add = button;
	gtk_widget_set_sensitive(button, FALSE);

	button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_box_pack_start(GTK_BOX(hbox_bottom), button, TRUE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", 
			G_CALLBACK(cb_quit_clicked), &cb_quit_clicked_data);

// Finish initializing the tree_view structure.
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view)))
			, "changed", G_CALLBACK(cb_selection_changed), &cb_sc_data);
	gtk_box_pack_start(GTK_BOX(hbox_top), tree_view, FALSE, FALSE, 3);
	for (Problem_config_array::size_type i = 0; i < ct_conf.problems.size(); i ++)
	{
		Problem_config &cur = ct_conf.problems[i];
		tree_view_add_problem(tree_view, cur.name.c_str());
		tree_view_add_points(tree_view, i, cur.points.size());
	}

	gtk_widget_show_all(win);

	gtk_main();

	ct_conf.write_conf(conf_file);
	if (!conf_file.save())
	{
		while (true)
		{
			char msg[CSTRING_MAX_LEN];
			snprintf(msg, CSTRING_MAX_LEN, "%s\nDetails:\n%s", _(
"Failed to save the configuration file.\nPlease choose another path, or all the changes will be lost.\n\
Would you like to choose another file path?"), strerror(errno));
			if (!confirm_dialog(NULL, msg, GTK_MESSAGE_ERROR))
				break;
			const char *path = file_selection_dialog(
					_("Please select another configuration file"), NULL, "*.cfg");
			if (path == NULL || conf_file.save_as(path))
				break;
		}
	}
}

void cb_quit_clicked(GtkButton *button, gpointer data0)
{
	Cb_quit_clicked_data *data = 
		static_cast<Cb_quit_clicked_data*>(data0);
	simulate_tree_view_change(data->tree_view);
	set<string> exist;
	for (Problem_config_array::const_iterator i = data->ptr_ct_conf->problems.begin(); 
			i != data->ptr_ct_conf->problems.end(); i ++)
	{
		if (exist.find(i->name) != exist.end())
		{
			char msg[CSTRING_MAX_LEN];
			snprintf(msg, CSTRING_MAX_LEN, _(
"More than one problem use the same name '%s'.\n\
Please change their names, so they are different from each other."), i->name.c_str());
			warning_dialog(main_window, msg);
			return;
		}
		exist.insert(i->name);
	}
	gtk_widget_destroy(GTK_WIDGET(main_window));
}

void cb_auto_add_test_points_clicked(GtkButton *button, gpointer data0)
{
	Cb_add_delete_data *data = 
		static_cast<Cb_add_delete_data*>(data0);
	assert(data->selection[0] != -1);
	int pos = data->selection[0];
	data->selection[0] = -1;
	data->selection[1] = -1;
	auto_add_test_points_local(data->tree_view, pos,
			data->ptr_ct_conf->problems[pos]);
}

void cb_auto_configure_clicked(GtkButton *button, gpointer data0)
{
	Cb_add_delete_data *data = 
		static_cast<Cb_add_delete_data*>(data0);
	Str_array dir;
	if (!list_dir_in_dir(DEFAULT_DATA_DIR, dir))
	{
		char tmp[CSTRING_MAX_LEN];
		snprintf(tmp, CSTRING_MAX_LEN, 
				_("Failed to list the directory.\nDetails:\n%s"), strerror(errno));
		warning_dialog(main_window, tmp);
		return;
	}
	int count = data->ptr_ct_conf->problems.size();
	for (int i = 0; i < count; i ++)
	{
		data->selection[0] = -1;
		data->selection[1] = -1;
		tree_view_remove_problem(data->tree_view, 0);
	}
	data->ptr_ct_conf->problems.resize(dir.size());
	char msg[CSTRING_MAX_LEN];
	for (Str_array::size_type i = 0; i < dir.size(); i ++)
	{
		snprintf(msg, CSTRING_MAX_LEN,
				_("Directory '%s' is found. Assume in it is a problem configuration."),
				dir[i].c_str());
		message_dialog(main_window, msg);
		tree_view_add_problem(data->tree_view, dir[i].c_str());
		data->selection[0] = -1;
		data->selection[1] = -1;
		data->ptr_ct_conf->problems[i].init(dir[i].c_str());
		auto_add_test_points_local(data->tree_view, i,
				data->ptr_ct_conf->problems[i]);
	}
}

#define GET_COMMON_INFO(_np_) \
int time;\
long rlimit_as, rlimit_data, rlimit_stack;\
double score;\
do \
{ \
	string in("1");\
	input_dialog(main_window, _("Please enter the time limit[s] for every point:"), in);\
	if (sscanf(in.c_str(), "%d", &time) == EOF)\
		time = 1;\
	char tmp[CSTR_NUM_MAX_LEN]; \
	snprintf(tmp, CSTR_NUM_MAX_LEN, "%lf", 100.0 / (_np_)); \
	in.assign(tmp); \
	input_dialog(main_window, _("Please enter the score for every point:"), in);\
	if (sscanf(in.c_str(), "%lf", &score) == EOF)\
		score = 10;\
	in = "-1";\
	input_dialog(main_window, _("Please enter the rlimit_as for every point:"), in);\
	if (sscanf(in.c_str(), "%ld", &rlimit_as) == EOF)\
		rlimit_as = -1;\
	in = "-1";\
	input_dialog(main_window, _("Please enter the rlimit_data for every point:"), in);\
	if (sscanf(in.c_str(), "%ld", &rlimit_data) == EOF)\
		rlimit_data = -1;\
	in = "-1";\
	input_dialog(main_window, _("Please enter the rlimit_stack for every point:"), in);\
	if (sscanf(in.c_str(), "%ld", &rlimit_stack) == EOF)\
		rlimit_stack = -1;\
} while (0)

void auto_add_test_points_local(GtkWidget *tree_view, int prob_num, Problem_config &pconf)
{
	Point_info_array auto_result;
	char data_dir[CSTRING_MAX_LEN];
	snprintf(data_dir, CSTRING_MAX_LEN, "%s/%s", DEFAULT_DATA_DIR, pconf.name.c_str());
	if (!auto_add_points(data_dir, auto_result))
	{
		if (global_error_msg[0])
		{
			error_dialog(main_window, global_error_msg);
			return;
		}
		char tmp[CSTRING_MAX_LEN];
		size_t cur_dir_size = 8;
		char *cur_dir = new char[cur_dir_size];
		while (getcwd(cur_dir, cur_dir_size) == NULL)
		{
			delete []cur_dir;
			cur_dir_size <<= 1;
			cur_dir = new char[cur_dir_size];
		}
		snprintf(tmp, CSTRING_MAX_LEN, _(
"Failed to automatically add test points.\n\
You should put the input and output files in the directory '%s/%s'.\n\
And there should not be any extra files."), cur_dir, data_dir);
		delete []cur_dir;
		warning_dialog(main_window, tmp);
		simulate_tree_view_change(tree_view);
		return;
	}
	GET_COMMON_INFO(auto_result.size());
	int old_npoint = pconf.points.size(), new_npoint = auto_result.size();
	pconf.points.resize(new_npoint);
	string prefix(data_dir);	
	prefix.append("/");
	for (int i = 0; i < new_npoint; i ++)
	{
		Test_point &tp = pconf.points[i];
		tp.time = time;
		tp.rlimit_as = rlimit_as;
		tp.rlimit_data = rlimit_data;
		tp.rlimit_stack = rlimit_stack;
		tp.score = score;
		tp.input = prefix;
		tp.input.append(auto_result[i].input);
		tp.output = prefix;
		tp.output.append(auto_result[i].output);
	}
	if (old_npoint > new_npoint)
		tree_view_remove_points(tree_view, prob_num, old_npoint - new_npoint);
	else tree_view_add_points(tree_view, prob_num, new_npoint - old_npoint);
}

void cb_add_test_points_clicked(GtkButton *button, gpointer data0)
{
	Cb_add_delete_data *data = 
		static_cast<Cb_add_delete_data*>(data0);
	assert(data->selection[0] != -1);
	char tmp[CSTRING_MAX_LEN];
	const char *probname = data->ptr_ct_conf->problems[data->selection[0]].name.c_str();
	snprintf(tmp, CSTRING_MAX_LEN, "data/%s/%s%%.in", probname, probname);
	string input_pattern(tmp);
	input_dialog(main_window, _("Please enter the pattern for standard input files:"), input_pattern);
	if (input_pattern.empty())
		return;
	snprintf(tmp, CSTRING_MAX_LEN, "data/%s/%s%%.out", probname, probname);
	string output_pattern(tmp);
	input_dialog(main_window, _("Please enter the pattern for standard output files:"), output_pattern);
	if (output_pattern.empty())
		return;
	string vars("1 2 3 4 5 6 7 8 9 10");
	if (vars.empty())
		return;
	snprintf(tmp, CSTRING_MAX_LEN, _("Please enter the values that are separated by a space to replace the '%%' mark:"));
	input_dialog(main_window, tmp, vars);
	istringstream strin(vars);
	int count = 0;
	Test_point_array &tps = data->ptr_ct_conf->problems[data->selection[0]].points;
	string val;
	while (strin >> val)
	{
		tps.push_back(Test_point());
		Test_point &tp = *(tps.rbegin());
		string str(input_pattern);
		string::size_type pos;
		while ((pos = str.find('%', 0)) != string::npos)
			str.replace(pos, 1, val);
		tp.input = str;
		str = output_pattern;
		while ((pos = str.find('%', 0)) != string::npos)
			str.replace(pos, 1, val);
		tp.output = str;
		count ++;
	}
	GET_COMMON_INFO(count);
	for (Test_point_array::iterator iter = tps.begin(); iter != tps.end(); iter ++)
	{
		iter->time = time;
		iter->rlimit_as = rlimit_as;
		iter->rlimit_data = rlimit_data;
		iter->rlimit_stack = rlimit_stack;
		iter->score = score;
	}
	tree_view_add_points(data->tree_view, data->selection[0], count);
}
#undef GET_COMMON_INFO

void cb_delete_clicked(GtkButton *button, gpointer data0)
{
	Cb_add_delete_data *data = 
		static_cast<Cb_add_delete_data*>(data0);
	if (data->selection[0] == -1)
		return;
	Problem_config_array &prob = data->ptr_ct_conf->problems;
	int pos = data->selection[0];
	if (data->selection[1] == -1)
	{
		char tmp[CSTRING_MAX_LEN];
		snprintf(tmp, CSTRING_MAX_LEN, _("Are you sure to delete the problem '%s'?"),
				prob[pos].name.c_str());
		if (confirm_dialog(main_window, tmp))
		{
			prob.erase(prob.begin() + pos);
			data->selection[0] = -1;
			data->selection[1] = -1;
			tree_view_remove_problem(data->tree_view, pos);
		}
	}else
	{
		Test_point_array &points = prob[pos].points;
		points.erase(points.begin() + data->selection[1]);
		data->selection[0] = -1;
		data->selection[1] = -1;
		tree_view_remove_points(data->tree_view, pos, 1);
	}
}

void cb_add_problem_clicked(GtkButton *button, gpointer data0)
{
	Cb_add_delete_data *data = 
		static_cast<Cb_add_delete_data*>(data0);
	string name;
	input_dialog(main_window, _("Please enter the problem name"), name);
	if (name.empty())
		return;
	Problem_config_array &prob = data->ptr_ct_conf->problems;
	for (Problem_config_array::iterator iter = prob.begin(); iter != prob.end(); iter ++)
		if (iter->name == name)
		{
			warning_dialog(main_window, _("This name already exists."));
			return;
		}
	prob.push_back(Problem_config());
	prob.rbegin()->init(name.c_str());
	tree_view_add_problem(data->tree_view, name.c_str());
}

GtkWidget* create_prob_info_frame(Prob_info_frame_data &data, Cb_add_delete_data *ptr_add_del_data)
{
	GtkWidget *frame = gtk_frame_new(_("Problem Information"));

	GtkWidget *table = gtk_table_new(7, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
#define ATTACH(_widget_, _l_, _r_, _t_, _b_) \
	gtk_table_attach(GTK_TABLE(table),  \
			(_widget_), (_l_), (_r_), (_t_), (_b_),\
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), (GtkAttachOptions)(0), 3, 3);
#define ATTACH_X_NO_OPT(_widget_, _l_, _r_, _t_, _b_) \
	gtk_table_attach(GTK_TABLE(table),  \
			(_widget_), (_l_), (_r_), (_t_), (_b_),\
			(GtkAttachOptions)(0), (GtkAttachOptions)(0), 3, 3);
#define ENTRY_WITH_LABEL(_label_, _row_, _e_id_) \
	ATTACH_X_NO_OPT(gtk_label_new((_label_)), 0, 1, (_row_), (_row_) + 1); \
	ATTACH(data.entry_##_e_id_ = gtk_entry_new(), 1, 2, (_row_), (_row_) + 1);

	ENTRY_WITH_LABEL(_("Problem Name:"), 0, name);
	ENTRY_WITH_LABEL(_("Source File Name:"), 1, source);
	ENTRY_WITH_LABEL(_("Input File Name:"), 2, input);
	ENTRY_WITH_LABEL(_("Output File Name:"), 3, output);

	g_signal_connect(G_OBJECT(data.entry_name), "key-release-event",
			G_CALLBACK(cb_prob_name_entry_changed), ptr_add_del_data);

	data.check_button_user_cmp = gtk_check_button_new_with_label(_("Standard Verifier"));
	ATTACH_X_NO_OPT(data.check_button_user_cmp, 0, 1, 4, 5);

	ENTRY_WITH_LABEL(_("Customized Verifier"), 5, cmp_func);
	g_signal_connect(G_OBJECT(data.check_button_user_cmp), "toggled",
			G_CALLBACK(cb_prif_togglebutton_toggled), data.entry_cmp_func);

	GtkWidget *label = gtk_label_new(
			_("For more information about the configuration, please refer to injuslin's manual page."));
	ATTACH(label, 0, 2, 6, 7);

	return frame;
#undef ATTACH
#undef ATTACH_X_NO_OPT
#undef ENTRY_WITH_LABEL
}

gboolean cb_prob_name_entry_changed(GtkWidget *entry, GdkEventKey *key, gpointer data0)
{
	Cb_add_delete_data *data = 
		static_cast<Cb_add_delete_data*>(data0);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree_view));
	GtkTreeModel *model;
	GtkTreeIter iter;
	assert(gtk_tree_selection_get_selected(selection, &model, &iter) == TRUE);
	gtk_tree_store_set(GTK_TREE_STORE(model), &iter, 0, 
			gtk_entry_get_text(GTK_ENTRY(entry)), -1);
	return FALSE;
}

void cb_prif_togglebutton_toggled(GtkToggleButton *button, gpointer entry)
{
	gtk_widget_set_sensitive(GTK_WIDGET(entry), !gtk_toggle_button_get_active(button));
}

GtkWidget* create_point_info_frame(Point_info_frame_data &data)
{
	GtkWidget *frame = gtk_frame_new(_("Test Point Information"));

	GtkWidget *table = gtk_table_new(5, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), table);
#define ATTACH(_widget_, _l_, _r_, _t_, _b_) \
	gtk_table_attach(GTK_TABLE(table),  \
			(_widget_), (_l_), (_r_), (_t_), (_b_),\
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), (GtkAttachOptions)(0), 3, 3);
#define ATTACH_X_NO_OPT(_widget_, _l_, _r_, _t_, _b_) \
	gtk_table_attach(GTK_TABLE(table),  \
			(_widget_), (_l_), (_r_), (_t_), (_b_),\
			(GtkAttachOptions)(0), (GtkAttachOptions)(0), 3, 3);
#define ENTRY_WITH_LABEL(_label_, _row_, _e_id_) \
	ATTACH_X_NO_OPT(gtk_label_new((_label_)), 0, 1, (_row_), (_row_) + 1); \
	ATTACH(data.entry_##_e_id_ = gtk_entry_new(), 1, 2, (_row_), (_row_) + 1);

	ENTRY_WITH_LABEL(_("Input File Name:"), 0, input);
	ENTRY_WITH_LABEL(_("Output File Name:"), 1, output);
	ENTRY_WITH_LABEL(_("Score:"), 2, score);

	gtk_entry_set_max_length(
			GTK_ENTRY(data.entry_score), CSTR_NUM_MAX_LEN - 1);

	GtkWidget *label = gtk_label_new(_("Time Limit[s]:"));
	ATTACH_X_NO_OPT(label, 0, 1, 3, 4);

	GtkAdjustment *adj = GTK_ADJUSTMENT(
			gtk_adjustment_new(1.0, 1.0, 1000.0, 1.0, 100.0, 0.0));
	data.sb_time = gtk_spin_button_new(adj, 0.0, 0);
	gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(data.sb_time), FALSE);
	ATTACH(data.sb_time, 1, 2, 3, 4);
	gtk_entry_set_max_length(
			GTK_ENTRY(data.sb_time), CSTR_NUM_MAX_LEN - 1);

	GtkWidget *frame_rlimit = gtk_frame_new(_("Resource Limit (-1 means unlimited)"));
	ATTACH(frame_rlimit, 0, 2, 4, 5);

	table = gtk_table_new(3, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(frame_rlimit), table);

	ENTRY_WITH_LABEL(_("rlimit_as:"), 0, rl_as);
	ENTRY_WITH_LABEL(_("rlimit_data:"), 1, rl_data);
	ENTRY_WITH_LABEL(_("rlimit_stack:"), 2, rl_stack);
	gtk_entry_set_max_length(
			GTK_ENTRY(data.entry_rl_as), CSTR_NUM_MAX_LEN - 1);
	gtk_entry_set_max_length(
			GTK_ENTRY(data.entry_rl_data), CSTR_NUM_MAX_LEN - 1);
	gtk_entry_set_max_length(
			GTK_ENTRY(data.entry_rl_stack), CSTR_NUM_MAX_LEN - 1);

	return frame;
#undef ATTACH
#undef ATTACH_X_NO_OPT
#undef ENTRY_WITH_LABEL
}

void cb_general_options_clicked(GtkButton *button, gpointer conf)
{
	modify_init(*static_cast<Contest_config*>(conf));
}

GtkWidget* create_view_and_model()
{
	GtkWidget *view = gtk_tree_view_new();

	GtkTreeViewColumn *col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Problems"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, 
			"text", 0);

	GtkTreeModel *model = GTK_TREE_MODEL(gtk_tree_store_new(1, G_TYPE_STRING));
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	g_object_unref(model); 

	return view;
}

void cb_selection_changed(GtkTreeSelection *selection, gpointer data0)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	Cb_selection_changed_data *data = 
		static_cast<Cb_selection_changed_data*> (data0);
	data->update_ct_conf();
	int nb_pn;
	const char *label_delete;
	gboolean sen;
	if (gtk_tree_selection_get_selected(
				selection, &model, &iter))
	{
		gchar *path = gtk_tree_model_get_string_from_iter(
				model, &iter);
		int pos = 0;
		data->selection[0] = 0;
		data->selection[1] = 0;
		for (gchar *i = path; *i; i ++)
			if (*i == ':')
				pos ++;
			else data->selection[pos] = data->selection[pos] * 10 + *i - '0';
		if (pos)
		{
			nb_pn = data->nbp_point;
			label_delete = _("_Delete Test Point");
		}else 
		{
			nb_pn = data->nbp_problem;
			label_delete = _("_Delete Problem");
			data->selection[1] = -1;
		}
		sen = TRUE;
		g_free(path);
	}else
	{
		nb_pn = data->nbp_none;
		label_delete = _("_Delete Problem");
		data->selection[0] = -1;
		data->selection[1] = -1;
		sen = FALSE;
	}
	gtk_notebook_set_current_page(
			GTK_NOTEBOOK(data->notebook_info), nb_pn);
	gtk_button_set_label(
			GTK_BUTTON(data->button_delete), label_delete);
	gtk_widget_set_sensitive(
			data->button_delete, sen);
	gtk_widget_set_sensitive(
			data->button_add, sen);
	gtk_widget_set_sensitive(
			data->button_auto_add, sen);
	data->update_info_frame();
}

void Cb_selection_changed_data::update_ct_conf()
{
	if (selection[0] == -1)
		return;
	Problem_config &cur_prob = ptr_ct_conf->problems[selection[0]];
	if (selection[1] == -1)
	{
#define UPDATE(_e_id_) \
		cur_prob._e_id_.assign(gtk_entry_get_text(GTK_ENTRY(ptr_prif->entry_##_e_id_)));
		UPDATE(name);
		UPDATE(input);
		UPDATE(output);
		UPDATE(source);
		if (gtk_toggle_button_get_active(
					GTK_TOGGLE_BUTTON(ptr_prif->check_button_user_cmp)))
		{
			cur_prob.compare_func = "s";
		}else
		{
			cur_prob.compare_func = "u";
			cur_prob.compare_func.append(
					gtk_entry_get_text(GTK_ENTRY(ptr_prif->entry_cmp_func)));
		}
#undef UPDATE
	}else
	{
		Test_point &cur_point = cur_prob.points[selection[1]];
		cur_point.time = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(ptr_poif->sb_time));
#define UPDATE(_1_, _2_) \
		sscanf(gtk_entry_get_text(GTK_ENTRY(ptr_poif->entry_##_1_)), \
				"%ld", &(cur_point._2_));
		UPDATE(rl_as, rlimit_as);
		UPDATE(rl_data, rlimit_data);
		UPDATE(rl_stack, rlimit_stack);
#undef UPDATE
		sscanf(gtk_entry_get_text(GTK_ENTRY(ptr_poif->entry_score)), "%lf", &(cur_point.score));
#define UPDATE(_e_id_) \
		cur_point._e_id_.assign(gtk_entry_get_text(GTK_ENTRY(ptr_poif->entry_##_e_id_)));
		UPDATE(input);
		UPDATE(output);
#undef UPDATE
	}
}

void Cb_selection_changed_data::update_info_frame()
{
	if (selection[0] == -1)
		return;
	Problem_config &cur_prob = ptr_ct_conf->problems[selection[0]];
	if (selection[1] == -1)
	{
#define UPDATE(_e_id_) \
		gtk_entry_set_text(GTK_ENTRY(ptr_prif->entry_##_e_id_), \
				cur_prob._e_id_.c_str());
		UPDATE(name);
		UPDATE(input);
		UPDATE(output);
		UPDATE(source);
		if (cur_prob.compare_func[0] == 's')
		{
			gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(ptr_prif->check_button_user_cmp), TRUE);
			gtk_entry_set_text(GTK_ENTRY(ptr_prif->entry_cmp_func), "");
			gtk_widget_set_sensitive(ptr_prif->entry_cmp_func, FALSE);
		}else
		{
			gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(ptr_prif->check_button_user_cmp), FALSE);
			gtk_entry_set_text(GTK_ENTRY(ptr_prif->entry_cmp_func), cur_prob.compare_func.c_str() + 1);
			gtk_widget_set_sensitive(ptr_prif->entry_cmp_func, TRUE);
		}
#undef UPDATE
	} else
	{
		Test_point &cur_point = cur_prob.points[selection[1]];
#define UPDATE(_1_, _2_) \
		gtk_entry_set_text(GTK_ENTRY(ptr_poif->_1_), \
				cur_point._2_.c_str());
		char tmp[CSTR_NUM_MAX_LEN];
#define UPDATE_long(_1_, _2_) \
		snprintf(tmp, CSTR_NUM_MAX_LEN, "%ld", cur_point._2_); \
		gtk_entry_set_text(GTK_ENTRY(ptr_poif->_1_), tmp);
#define UPDATE_double(_1_, _2_) \
		snprintf(tmp, CSTR_NUM_MAX_LEN, "%.3f", cur_point._2_); \
		gtk_entry_set_text(GTK_ENTRY(ptr_poif->_1_), tmp);
		gtk_spin_button_set_value(
				GTK_SPIN_BUTTON(ptr_poif->sb_time), cur_point.time);
		UPDATE_long(entry_rl_as, rlimit_as);
		UPDATE_long(entry_rl_data, rlimit_data);
		UPDATE_long(entry_rl_stack, rlimit_stack);
		UPDATE_double(entry_score, score);
		UPDATE(entry_input, input);
		UPDATE(entry_output, output);
#undef UPDATE
#undef UPDATE_long
#undef UPDATE_double
	}
}

void tree_view_add_problem(GtkWidget *view, const char *str)
{
	GtkTreeStore *store = GTK_TREE_STORE(
			gtk_tree_view_get_model(GTK_TREE_VIEW(view)));
	GtkTreeIter iter;
	gtk_tree_store_append(store, &iter, NULL);
	gtk_tree_store_set(store, &iter, 0, str, -1);
	simulate_tree_view_change(view);
}

void tree_view_add_points(GtkWidget *view, int prob_num, int pcount)
{
	if (pcount != 0)
	{
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		GtkTreeIter iter;
		assert(gtk_tree_model_iter_nth_child(model, &iter, NULL, prob_num) == TRUE);
		int nch = gtk_tree_model_iter_n_children(model, &iter);
		GtkTreeStore *store = GTK_TREE_STORE(model);

		GtkTreeIter child;
		for (int i = 0; i < pcount; i ++)
		{
			gtk_tree_store_append(store, &child, &iter);
			char tmp[CSTR_INT_MAX_LEN];
			sprintf(tmp, "%d", ++ nch);
			gtk_tree_store_set(store, &child, 0, tmp, -1);
		}
	}
	simulate_tree_view_change(view);
}

void tree_view_remove_problem(GtkWidget *view, int prob_num)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
	GtkTreeIter iter;
	assert(gtk_tree_model_iter_nth_child(model, &iter, NULL, prob_num) == TRUE);
	GtkTreeStore *store = GTK_TREE_STORE(model);
	gtk_tree_store_remove(store, &iter);
	simulate_tree_view_change(view);
}

void tree_view_remove_points(GtkWidget *view, int prob_num, int pcount)
{
	if (pcount != 0)
	{
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));
		GtkTreeIter iter;
		assert(gtk_tree_model_iter_nth_child(model, &iter, NULL, prob_num) == TRUE);
		int nch = gtk_tree_model_iter_n_children(model, &iter);
		assert(pcount <= nch);

		int pos = nch - pcount;
		GtkTreeIter child;
		GtkTreeStore *store = GTK_TREE_STORE(model);
		for (int i = 0; i < pcount; i ++)
		{
			assert(gtk_tree_model_iter_nth_child(model, &child, &iter, pos) == TRUE);
			gtk_tree_store_remove(store, &child);
		}
	}
	simulate_tree_view_change(view);
}

void simulate_tree_view_change(GtkWidget *view)
{
	g_signal_emit_by_name(
			G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(view))), 
			"changed",
			gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), NULL);
}

void modify_init(Contest_config &conf)
{
	Cb_modify_contest_config_data *pdata = new Cb_modify_contest_config_data;
	pdata->ptr_ct_conf = &conf;
	pdata->ptr_orig_map = &conf.compiler;
	for (Str_map::iterator i = conf.compiler.begin(); i != conf.compiler.end(); i ++)
	{
		pdata->compiler_ext.push_back(i->first);
		pdata->compiler_command.push_back(i->second);
	}

	GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	pdata->main_win = GTK_WINDOW(win);
	g_signal_connect(G_OBJECT(win), "destroy", 
			G_CALLBACK(cb_modify_contest_config_destroy), pdata);
	gtk_container_set_border_width(GTK_CONTAINER(win), 10);
	gtk_window_set_title(GTK_WINDOW(win), _("General Options"));
	gtk_window_set_transient_for(GTK_WINDOW(win), main_window);
	gtk_widget_set_sensitive(GTK_WIDGET(main_window), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

	GtkWidget *vbox_back = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(win), vbox_back);

	GtkWidget *vbox_top = gtk_vbox_new(FALSE, 3),
			  *hbox_bottom = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox_back), vbox_top, TRUE, TRUE, 3);
	gtk_box_pack_end(GTK_BOX(vbox_back), hbox_bottom, FALSE, FALSE, 3);

	GtkWidget *button_ok = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_box_pack_end(GTK_BOX(hbox_bottom), button_ok, FALSE, FALSE, 3);
	g_signal_connect(G_OBJECT(button_ok), "clicked", 
			G_CALLBACK(cb_modify_contest_config_ok), pdata);
	g_signal_connect_swapped(G_OBJECT(button_ok), "clicked",
			G_CALLBACK(gtk_widget_destroy), win);

	GtkWidget *button_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_end(GTK_BOX(hbox_bottom), button_cancel, FALSE, FALSE, 3);
	g_signal_connect_swapped(G_OBJECT(button_cancel), "clicked",
			G_CALLBACK(gtk_widget_destroy), win);

	pdata->entry_name = new_entry_with_label(
			GTK_BOX(vbox_top), _("Contest Name:"), conf.contest_name.c_str());
	pdata->entry_srcdir = new_entry_with_label(
			GTK_BOX(vbox_top), _("Source file directory:"), conf.source_dir.c_str());

	GtkWidget *frame = gtk_frame_new(_("Compiler Options"));
	GtkWidget *frame_table = gtk_table_new(3, 3, FALSE);
	gtk_container_add(GTK_CONTAINER(frame), frame_table);
	gtk_box_pack_end(GTK_BOX(vbox_top), frame, FALSE, FALSE, 3);

#define ATTACH(_widget_, _l_, _r_, _t_, _b_) \
	gtk_table_attach(GTK_TABLE(frame_table),  \
			(_widget_), (_l_), (_r_), (_t_), (_b_),\
			(GtkAttachOptions)(GTK_FILL | GTK_EXPAND), (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), 3, 3);
#define ATTACH_X_NO_OPT(_widget_, _l_, _r_, _t_, _b_) \
	gtk_table_attach(GTK_TABLE(frame_table),  \
			(_widget_), (_l_), (_r_), (_t_), (_b_),\
			(GtkAttachOptions)(0), (GtkAttachOptions)(GTK_FILL | GTK_EXPAND), 3, 3);
	GtkWidget *label = gtk_label_new(_("File Extension"));
	ATTACH(label, 0, 1, 0, 1);

	label = gtk_label_new(_("Compiler Command"));
	ATTACH(label, 1, 3, 0, 1);

	GtkWidget *combo_box = gtk_combo_box_new_text();
	pdata->combo_box_ext = combo_box;
	pdata->combo_box_prev_select = 0;
	ATTACH(combo_box, 0, 1, 1, 2);
	for (Str_array::iterator i = pdata->compiler_ext.begin(); i != pdata->compiler_ext.end(); i ++)
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo_box), i->c_str());
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo_box), 0);
	g_signal_connect(G_OBJECT(combo_box), "changed",
			G_CALLBACK(cb_modify_contest_config_combo_box_changed), pdata);

	GtkWidget *entry = gtk_entry_new();
	pdata->entry_command = entry;
	ATTACH(entry, 1, 3, 1, 2);
	gtk_entry_set_text(GTK_ENTRY(entry), pdata->compiler_command.begin()->c_str());

	GtkWidget *button = gtk_button_new_with_mnemonic(_("_Add"));
	ATTACH_X_NO_OPT(button, 0, 1, 2, 3);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_modify_contest_config_add_clicked), pdata);

	button = gtk_button_new_with_mnemonic(_("_Remove"));
	ATTACH_X_NO_OPT(button, 1, 2, 2, 3);
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(cb_modify_contest_config_remove_clicked), pdata);
#undef ATTACH
#undef ATTACH_X_NO_OPT

	gtk_widget_show_all(win);
}

void cb_modify_contest_config_add_clicked(GtkButton *button, gpointer data0)
{
	Cb_modify_contest_config_data *pdata = 
		static_cast<Cb_modify_contest_config_data*>(data0);
	string ext;
	input_dialog(pdata->main_win, _("Please enter the file extension"), ext);
	if (ext.empty())
	{
		warning_dialog(pdata->main_win, _("Sorry, you entered an empty string."));
		return;
	}
	pdata->compiler_ext.push_back(ext);
	pdata->compiler_command.push_back("");
	gtk_combo_box_append_text(GTK_COMBO_BOX(pdata->combo_box_ext), ext.c_str());
	gtk_combo_box_set_active(GTK_COMBO_BOX(pdata->combo_box_ext),
			pdata->compiler_ext.size() - 1);
	gtk_widget_grab_focus(pdata->entry_command);
}

void cb_modify_contest_config_remove_clicked(GtkButton *button, gpointer data0)
{
	Cb_modify_contest_config_data *pdata = 
		static_cast<Cb_modify_contest_config_data*>(data0);
	if (pdata->compiler_ext.size() == 1)
	{
		warning_dialog(pdata->main_win, _("Cannot remove the last item."));
		return;
	}
	if (pdata->combo_box_prev_select == -1)
	{
		warning_dialog(pdata->main_win, _("No item is selected."));
		return;
	}
	pdata->compiler_command.erase(
			pdata->compiler_command.begin() + pdata->combo_box_prev_select);
	pdata->compiler_ext.erase(
			pdata->compiler_ext.begin() + pdata->combo_box_prev_select);
	gtk_combo_box_remove_text(GTK_COMBO_BOX(pdata->combo_box_ext), pdata->combo_box_prev_select);
	pdata->combo_box_prev_select = -1;
	gtk_combo_box_set_active(GTK_COMBO_BOX(pdata->combo_box_ext), 0);
}

void cb_modify_contest_config_combo_box_changed(GtkComboBox *widget, gpointer data0)
{
	Cb_modify_contest_config_data *pdata = 
		static_cast<Cb_modify_contest_config_data*>(data0);
	if (pdata->combo_box_prev_select != -1)
	{
		pdata->compiler_command[pdata->combo_box_prev_select].assign
			(gtk_entry_get_text(GTK_ENTRY(pdata->entry_command)));
	}
	int pos = gtk_combo_box_get_active(widget);
	if (pos == -1)
		gtk_entry_set_text(GTK_ENTRY(pdata->entry_command), "");
	else gtk_entry_set_text(GTK_ENTRY(pdata->entry_command), 
			pdata->compiler_command[pos].c_str());
	pdata->combo_box_prev_select = pos;
}

void cb_modify_contest_config_ok(GtkButton *button, gpointer data0)
{
	Cb_modify_contest_config_data *pdata = 
		static_cast<Cb_modify_contest_config_data*>(data0);
	if (pdata->combo_box_prev_select != -1)
	{
		pdata->compiler_command[pdata->combo_box_prev_select].assign
			(gtk_entry_get_text(GTK_ENTRY(pdata->entry_command)));
	}

	pdata->ptr_ct_conf->contest_name.assign
		(gtk_entry_get_text(GTK_ENTRY(pdata->entry_name)));
	pdata->ptr_ct_conf->source_dir.assign
		(gtk_entry_get_text(GTK_ENTRY(pdata->entry_srcdir)));

	set_main_window_title(pdata->ptr_ct_conf->contest_name.c_str());

	pdata->ptr_orig_map->clear();
	for (Str_array::size_type i = 0; i < pdata->compiler_ext.size(); i ++)
		(*pdata->ptr_orig_map)[pdata->compiler_ext[i]] = 
			pdata->compiler_command[i];
}

void cb_modify_contest_config_destroy(GtkObject *ob, gpointer data0)
{
	Cb_modify_contest_config_data *pdata = 
		static_cast<Cb_modify_contest_config_data*>(data0);
	delete pdata;
	gtk_widget_set_sensitive(GTK_WIDGET(main_window), TRUE);
}

void set_default_conf(Conf &conf)
{
	conf.write("general", "name", "Default");
	conf.write("general", "src", "src/");
	conf.write("general", "problems_num", "0");
	conf.write("general", "VERSION", "1");
	conf.write("compiler", "cpp", "g++ %s.cpp -o %s");
	conf.write("compiler", "c", "gcc %s.c -o %s -lm");
	conf.write("compiler", "pas", "fpc %s.pas");
}

void set_main_window_title(const char *contest_name)
{
	char tmp[CSTRING_MAX_LEN];
	snprintf(tmp, CSTRING_MAX_LEN, "%s [%s]", contest_name, VERSION_INF);
	gtk_window_set_title(main_window, tmp);
}

