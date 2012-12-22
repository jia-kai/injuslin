/*
 * $File: main.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sat Oct 03 12:34:02 2009
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include <cstdlib>
#include <cstdio>
#include <string>
using namespace std;


#include "common.h"
#include "start_judge.h"
#include "configure_contest.h"
#include "gtk_addition.h"
#include "view_results.h"

typedef void (* Method_func) (const char *);
static Method_func func;
//----------------------Function Declaration----------------------------------
static void usage(const char *name);
static void main_win();
static void init_work_dir(string &path);
static void cb_configure_contest_clicked(GtkButton *button, gpointer win);
static void cb_start_judge_clicked(GtkButton *button, gpointer win);
static void cb_view_results_clicked(GtkButton *button, gpointer win);
static const char *str2cstr(const string &str);
	// return NULL if @str is empty
//----------------------------------------------------------------------------

#define REPORT_ERROR(_msg_) \
do \
{ \
	perror(_msg_); \
	fprintf(stderr, _("(At %s : %s : %d)\n"), __FILE__, __PRETTY_FUNCTION__, __LINE__); \
	exit(EXIT_FAILURE); \
} while(0)

int main(int argc, char *argv[])
{
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	int mode = 0;
	const int MODE_JUDGE = 1, MODE_QUIET_JUDGE = 2, MODE_REPORT = 4, MODE_RESULT = 8;
	string conf_path, quiet_judge_ct, quiet_judge_pr, report_path, result_ct;
	while (true)
	{
		static const struct option long_options[] = 
		{
			{"file", 1, NULL, 'f'},
			{"judge", 0, NULL, 'j'},
			{"quiet", 2, NULL, 'q'},
			{"report", 1, NULL, 'r'},
			{"result", 2, NULL, 'R'},
			{"help", 0, NULL, 'h'},
			{"version", 0, NULL, 'v'},
			{0, 0, 0, 0}
		};
		int c = getopt_long(argc, argv, "f:jq::r:R::hv",
							long_options, NULL);
		if (c == -1)
			break;
		switch (c)
		{
			case 'f':
				conf_path.assign(optarg);
				break;
			case 'j':
				mode |= MODE_JUDGE;
				break;
			case 'q':
				mode |= MODE_QUIET_JUDGE;
				if (optarg)
				{
					string *cur = &quiet_judge_ct;
					for (const char *ptr = optarg; *ptr; ptr ++)
						if (*ptr == ':')
							cur = &quiet_judge_pr;
						else cur->append(1, *ptr);
					if (cur != &quiet_judge_pr)
					{
						fprintf(stderr, "%s\n", _("Unrecognizable format of argument of option -q"));
						fprintf(stderr, "%s\n", _("Try -h option for more information."));
						exit(EXIT_FAILURE);
					}
				}
				break;
			case 'r':
				mode |= MODE_REPORT;
				report_path.assign(optarg);
				break;
			case 'R':
				mode |= MODE_RESULT;
				if (optarg)
					result_ct.assign(optarg);
				break;
			case 'v':
				printf(_("%s\nWritten by %s. \n"), VERSION_INF, AUTHOR);
				exit(EXIT_SUCCESS);
			default: // for '?' and 'h'
				usage(argv[0]);
				if (c == 'h')
					exit(EXIT_SUCCESS);
				exit(EXIT_FAILURE);
		}
	}
	if (mode)
	{
		if (conf_path.empty())
		{
			fprintf(stderr, "%s\n", _("Option -f not found."));
			fprintf(stderr, "%s\n", _("Try -h option for more information."));
			exit(EXIT_FAILURE);
		}
		if (!(setegid(getuid()) == 0 && seteuid(getuid()) == 0))
			REPORT_ERROR("setuid, setgid()");
		init_work_dir(conf_path);
		if (mode & MODE_QUIET_JUDGE)
			start_judge_quiet(conf_path.c_str(), str2cstr(quiet_judge_ct), str2cstr(quiet_judge_pr));
		else if (mode & MODE_JUDGE)
			start_judge(conf_path.c_str());
		if (mode & MODE_REPORT)
			export_results2html(conf_path.c_str(), report_path.c_str());
		if (mode & MODE_RESULT)
			export_results2stdout(conf_path.c_str(), str2cstr(result_ct));
		exit(EXIT_SUCCESS);
	}
	uid_t cur_uid = getuid();
	gid_t cur_gid = getgid();
	if (!(setresgid(cur_gid, cur_gid, cur_gid) == 0 && setresuid(cur_uid, cur_uid, cur_uid) == 0))
		REPORT_ERROR("setresgid, setresuid()");
	gtk_init(&argc, &argv);
	if (conf_path.empty())
	{
		const char *path = file_selection_dialog(
				_("Please select the configuration file"), NULL, "*.cfg");
		if (path == NULL)
			exit(EXIT_FAILURE);
		conf_path.assign(path);
	}
	init_work_dir(conf_path);
	main_win();
	if (func != NULL)
		func(conf_path.c_str());
}

void usage(const char *name)
{
	printf(_("Usage: %s [options]\nINformatics JUdge System for LINux\n\nOptions are:\n"), name);
	puts(_("\
-f, --file=file      Designate the configuration file. \n\
                     By default, a dialog will pop up to \n\
                     ask the user to select a file.\n"));
	puts(_("\
-j, --judge          Run in judge mode. \n\
                     Option -f required.\n"));
	puts(_("\
-q, --quiet[=ct:pr]  Run in quiet judge mode, \n\
                     meaning start judge process directly, \n\
                     without printing to standand output. \n\
                     If ct and pr are given, judge the \n\
                     contentant ct and problem pr. \n\
                     If either ct or pr is empty, \n\
                     it means all. (e.g. -q:test means \n\
                     judge the problem 'test' for all contestants.)\n\
                     Option -f required.\n"));
	puts(_("\
-r, --report=file    Output the judge results \n\
                     in HTML format to the file. \n\
                     Option -f required.\n"));
	puts(_("\
-R, --result[=ct]    Print the judge results \n\
                     to standand output. \n\
                     If ct is given, only print the \n\
                     results of contentant ct. \n\
                     Option -f required.\n"));
	puts(_("\
-h, --help           Display this help and exit.\n"));
	puts(_("\
-v, --version        Output version information and exit.\n"));
	printf(_("Please report bugs to <%s>\n"), PACKAGE_BUGREPORT);
}

void main_win()
{
	func = NULL;
	GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), VERSION_INF);
	gtk_window_set_default_size(GTK_WINDOW(win), 250, 0);
	gtk_container_set_border_width(GTK_CONTAINER(win), 10);
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	GtkWidget *vbox = gtk_vbox_new(TRUE, 10);

	GtkWidget *button = gtk_button_new_with_label(_("Configure the contest"));
	g_signal_connect(G_OBJECT(button), "clicked", 
					G_CALLBACK(cb_configure_contest_clicked), G_OBJECT(win));
	gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 3);

	button = gtk_button_new_with_label(_("Start judge"));
	g_signal_connect(G_OBJECT(button), "clicked", 
					G_CALLBACK(cb_start_judge_clicked), G_OBJECT(win));
	gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 3);

	button = gtk_button_new_with_label(_("View the judge results"));
	g_signal_connect(G_OBJECT(button), "clicked", 
					G_CALLBACK(cb_view_results_clicked), G_OBJECT(win));
	gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 3);

	gtk_container_add(GTK_CONTAINER(win), vbox);

	gtk_widget_show_all(win);

	gtk_main();
}

void init_work_dir(string &path)
{
	DIR *dir = opendir(".");
	if (dir == NULL)
		REPORT_ERROR("opendir");
	if ((fd_orig_work_dir = dirfd(dir)) == -1)
		REPORT_ERROR("dirfd");

	int pos = path.length() - 1;
	while (pos >= 0 && path[pos] != '/')
		pos --;
	if (pos < 0)
	{
		fd_config_work_dir = fd_orig_work_dir;
		return;
	}
	else dir = opendir(path.substr(0, pos + 1).c_str());
	if (dir == NULL)
		REPORT_ERROR("opendir");
	if ((fd_config_work_dir = dirfd(dir)) == -1)
		REPORT_ERROR("dirfd");
	if (fchdir(fd_config_work_dir))
		REPORT_ERROR("fchdir");
	path.erase(0, pos + 1);
}

void cb_configure_contest_clicked(GtkButton *button, gpointer win)
{
	gtk_object_destroy(GTK_OBJECT(win));
	func = configure_contest;
}

void cb_start_judge_clicked(GtkButton *button, gpointer win)
{
	const char *msg = _("\
Judge in graphics mode is currently not supported. \n\
Please view injuslin's manual page or run 'injuslin -h' from console for more information. \
");
	message_dialog(GTK_WINDOW(win), msg);
}

void cb_view_results_clicked(GtkButton *button, gpointer win)
{
	gtk_object_destroy(GTK_OBJECT(win));
	func = view_results;
}

const char *str2cstr(const string &str)
{
	if (str.empty())
		return NULL;
	return str.c_str();
}

