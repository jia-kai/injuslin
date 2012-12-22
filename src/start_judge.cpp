/*
 * $File: start_judge.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Jul 06 11:01:49 2011 +0800
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

#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <fstream>
using namespace std;

#define NEED_LIST_DIR
#include "common.h"
#include "start_judge.h"
#include "judge_core.h"
#include "structures.h"
#include "cui.h"
#include "filecmp.h"
using namespace cui;

typedef set<string> Str_set;

struct Shared_data
{
	bool stop_flag, sj_error;
		// if sj_error is true, global_error_msg will be set
	Window *detail_win;
	const char *sj_path;
	Judge_result_problem *ptr_result_prob;
	Judge_result_point_array *ptr_result_point;
	Shared_data(Window *dw) : stop_flag(false), sj_error(false), detail_win(dw), sj_path(NULL) {}
};

typedef vector<Problem_config_array::const_iterator> Iter_Problem_config_array;
//----------------------Function Declaration----------------------------------
static bool get_contestants(Str_array &result, const Contest_config &ct_conf);
	//return:
	//  false if an system error occurred, and errno is set.
static void get_problems(Iter_Problem_config_array &result, const Contest_config &ct_conf);
static void* thread_watch_stop(void *data);
static double cb_fc_verifier(double fullscore, const char *std_input_path,
		const char *std_output_path, const char *user_output_path, void *data);
static bool cb_judge_core(Judge_core_cb_info info, int addition_info, void *data);
static bool cb_judge_core_quiet(Judge_core_cb_info info, int addition_info, void *data);
//----------------------------------------------------------------------------

void start_judge(const char *conf_path)
{
	Conf conf_file(conf_path);
	Contest_config ct_conf;
	if (!ct_conf.init(conf_file))
	{
		fprintf(stderr, _("%s : %s : %d  : Failed to initialize the contest configuration.\nDetails:\n%s\n"), 
				__FILE__, __PRETTY_FUNCTION__, __LINE__, global_error_msg);
		return;
	}
	start_cui();
	Iter_Problem_config_array problems;
	get_problems(problems, ct_conf);
	if (problems.size() == 0)
	{
		end_cui();
		printf(_("No problem is selected. Exit.\n"));
		return;
	}
	Str_array cta_name;
	if (!get_contestants(cta_name, ct_conf))
	{
		end_cui();
		fprintf(stderr, _("%s : %s : %d  : Failed to get the contestants.\nDetails:\n%s\n"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__, strerror(errno));
		return;
	}
	if (cta_name.size() == 0)
	{
		end_cui();
		printf(_("No contestant is selected. Exit.\n"));
		return;
	}

// Initialize the judge process

	Window main_win;
	static const char *MAIN_WIN_MSG = "Judging... (Press 's' to stop)";
	int msg_nlines = get_str_nlines(MAIN_WIN_MSG);
	main_win.printf(MAIN_WIN_MSG);
	main_win.draw_hline(msg_nlines ++);

	Window win_proc_ct(1, 0, msg_nlines ++, 0, false);
	Window win_proc_prob(1, 0, msg_nlines ++, 0, false);

	Window detail_win(get_nlines() - msg_nlines, 0, msg_nlines, 0);
	detail_win.set_title("Details");
	detail_win.set_scrollable(true);

	Shared_data shared_data(&detail_win);
	pthread_t thread_watch_stop_pt;
	if (pthread_create(&thread_watch_stop_pt, NULL, thread_watch_stop, &shared_data))
	{
		end_cui();
		fprintf(stderr, _("%s : %s : %d  : Faied to create the thread.\n"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return;
	}
#define TO_RETURN \
	do \
	{ \
		end_cui(); \
		pthread_cancel(thread_watch_stop_pt); \
	}while(0)

	Str_set usable_prob_name;
	for (Problem_config_array::const_iterator iter = ct_conf.problems.begin(); iter != ct_conf.problems.end(); iter ++)
		usable_prob_name.insert(iter->name);

// Start the judge process
	for (Str_array::size_type i = 0; i < cta_name.size(); i ++)
	{
		win_proc_ct.clean();
		win_proc_ct.printf(_("Current contestant: %-10s (%d of %d)"), cta_name[i].c_str(), i + 1, cta_name.size());
		Conf cta_conf((ct_conf.source_dir + cta_name[i] + "/result.cfg").c_str());
		cta_conf.write("general", "VERSION", JUDGE_RESULT_CONF_VERSION);

		if (cta_conf.list_section_start("result"))
			for (Conf::Section_item *ptr = cta_conf.list_section_next(); ptr != NULL; ptr = cta_conf.list_section_next())
				if (usable_prob_name.find(ptr->item) == usable_prob_name.end())
					cta_conf.delitem("result", ptr->item);  // remove unused results

		detail_win.printf(_("\nJudging contestant '%s' ...\n"), cta_name[i].c_str());

		for (Str_array::size_type j = 0; j < problems.size(); j ++)
		{
			win_proc_prob.clean();
			win_proc_prob.printf(_("Current problem:    %-10s (%d of %d)"), problems[j]->name.c_str(), j + 1, problems.size());
			detail_win.printf(_("\n Judging problem '%s' ...\n"), problems[j]->name.c_str());
			update();
			Judge_result_problem result;
			shared_data.ptr_result_prob = &result;
			shared_data.ptr_result_point = &(result.points);
			bool judge_core_ok;
			if (problems[j]->compare_func[0] == 's')
			{
				judge_core_ok = judge_core(ct_conf, *problems[j], cta_name[i],
						filecmp_normal, NULL, result, cb_judge_core, &shared_data);
			}
			else
			{
				shared_data.sj_path = problems[j]->compare_func.c_str() + 1;
				judge_core_ok = judge_core(ct_conf, *problems[j], cta_name[i],
						cb_fc_verifier, &shared_data, result, cb_judge_core, &shared_data);
			}
			if (!judge_core_ok)
			{
				TO_RETURN;
				fprintf(stderr, _("%s : %s : %d  : An system error occurred.\nDetails:\n%s\n"),
						__FILE__, __PRETTY_FUNCTION__, __LINE__, global_error_msg);
				return;
			}
			if (shared_data.stop_flag)
			{
				TO_RETURN;
				if (shared_data.sj_error)
					fprintf(stderr, _("%s : %s : %d  : Failed to run the verifier.\nDetails:\n%s\n"),
							__FILE__, __PRETTY_FUNCTION__, __LINE__, global_error_msg);
				else printf(_("Judge process has been stopped.\n"));
				return;
			}
			string res_str;
			if (!result.convert2str(res_str))
			{
				TO_RETURN;
				fprintf(stderr, _("%s : %s : %d  : Failed to convert the judge result.\n"),
						__FILE__, __PRETTY_FUNCTION__, __LINE__);
				return;
			}
			cta_conf.write("result", problems[j]->name.c_str(), res_str);
		}

		if (!cta_conf.save())
		{
			TO_RETURN;
			fprintf(stderr, _("%s : %s : %d  : Failed to save the result file ('%s').\nDetails:\n%s\n"),
					__FILE__, __PRETTY_FUNCTION__, __LINE__, cta_conf.get_file_path(), strerror(errno));
			return;
		}
	}


	TO_RETURN;
	printf(_("Finished.\n"));
}

void start_judge_quiet(const char *conf_path, const char *ct, const char *pr)
{
	Conf conf_file(conf_path);
	Contest_config ct_conf;
	if (!ct_conf.init(conf_file))
	{
		fprintf(stderr, _("%s : %s : %d  : Failed to initialize the contest configuration.\nDetails:\n%s\n"), 
				__FILE__, __PRETTY_FUNCTION__, __LINE__, global_error_msg);
		return;
	}
	Str_array cta_name;
	if (ct != NULL)
	{
		string path(ct_conf.source_dir);
		path.append(ct);
		struct stat buf;
		if (stat(path.c_str(), &buf))
		{
			perror("stat");
			return;
		}
		if (!S_ISDIR(buf.st_mode))
		{
			fprintf(stderr, _("No such contestant '%s'.\n"), ct);
			return;
		}
		cta_name.push_back(string());
		cta_name.rbegin()->assign(ct);
	} else
	{
		if (!list_dir_in_dir(ct_conf.source_dir.c_str(), cta_name))
		{
			fprintf(stderr, _("%s : %s : %d  : Failed to get the contestants.\nDetails:\n%s\n"),
					__FILE__, __PRETTY_FUNCTION__, __LINE__, strerror(errno));
			return;
		}
	}
	if (cta_name.size() == 0)
	{
		fprintf(stderr, _("No contentant to judge.\n"));
		return;
	}
	Iter_Problem_config_array problems;
	if (pr != NULL)
	{
		bool ok = false;
		for (Problem_config_array::const_iterator iter = ct_conf.problems.begin(); 
				iter != ct_conf.problems.end(); iter ++)
		{
			if (iter->name == pr)
			{
				problems.push_back(iter);
				ok = true;
				break;
			}
		}
		if (!ok)
		{
			fprintf(stderr, _("No such problem '%s'.\n"), pr);
			return;
		}
	} else
	{
		for (Problem_config_array::const_iterator iter = ct_conf.problems.begin(); 
				iter != ct_conf.problems.end(); iter ++)
			problems.push_back(iter);
	}
	if (problems.size() == 0)
	{
		fprintf(stderr, _("No problem to judge.\n"));
		return;
	}

	Str_set usable_prob_name;
	for (Problem_config_array::const_iterator iter = ct_conf.problems.begin(); iter != ct_conf.problems.end(); iter ++)
		usable_prob_name.insert(iter->name);

// Start the judge process
	Shared_data shared_data(NULL);
	for (Str_array::size_type i = 0; i < cta_name.size(); i ++)
	{
		Conf cta_conf((ct_conf.source_dir + cta_name[i] + "/result.cfg").c_str());
		cta_conf.write("general", "VERSION", JUDGE_RESULT_CONF_VERSION);

		if (cta_conf.list_section_start("result"))
			for (Conf::Section_item *ptr = cta_conf.list_section_next(); ptr != NULL; ptr = cta_conf.list_section_next())
				if (usable_prob_name.find(ptr->item) == usable_prob_name.end())
					cta_conf.delitem("result", ptr->item);  // remove unused results

		for (Str_array::size_type j = 0; j < problems.size(); j ++)
		{
			Judge_result_problem result;
			bool judge_core_ok;
			if (problems[j]->compare_func[0] == 's')
			{
				judge_core_ok = judge_core(ct_conf, *problems[j], cta_name[i],
						filecmp_normal, NULL, result, cb_judge_core_quiet, &shared_data);
			}
			else
			{
				shared_data.sj_path = problems[j]->compare_func.c_str() + 1;
				judge_core_ok = judge_core(ct_conf, *problems[j], cta_name[i],
						cb_fc_verifier, &shared_data, result, cb_judge_core_quiet, &shared_data);
			}
			if (!judge_core_ok)
			{
				fprintf(stderr, _("%s : %s : %d  : An system error occurred.\nDetails:\n%s\n"),
						__FILE__, __PRETTY_FUNCTION__, __LINE__, global_error_msg);
				return;
			}
			if (shared_data.stop_flag)
			{
				fprintf(stderr, _("%s : %s : %d  : Failed to run the verifier.\nDetails:\n%s\n"),
						__FILE__, __PRETTY_FUNCTION__, __LINE__, global_error_msg);
				return;
			}
			string res_str;
			if (!result.convert2str(res_str))
			{
				fprintf(stderr, _("%s : %s : %d  : Failed to convert the judge result.\n"),
						__FILE__, __PRETTY_FUNCTION__, __LINE__);
				return;
			}
			cta_conf.write("result", problems[j]->name.c_str(), res_str);
		}

		if (!cta_conf.save())
		{
			fprintf(stderr, _("%s : %s : %d  : Failed to save the result file ('%s').\nDetails:\n%s\n"),
					__FILE__, __PRETTY_FUNCTION__, __LINE__, cta_conf.get_file_path(), strerror(errno));
			return;
		}
	}
}

bool cb_judge_core(Judge_core_cb_info info, int addition_info, void *data0)
{
	Shared_data *data = static_cast<Shared_data*>(data0);
	if (data->stop_flag)
		return true;
	double score;
	switch (info)
	{
		case JUDGE_CORE_CB_SRC_NOT_FOUND:
			data->detail_win->printf("  %s\n", _("Source file is not found."));
			break;
		case JUDGE_CORE_CB_SRC_FOUND:
			data->detail_win->printf("  %s%s\n", _("Found source file: "), (const char*)(addition_info));
			break;
		case JUDGE_CORE_CB_START_COMPILE:
			data->detail_win->printf("  %s\n", _("Compiling ..."));
			break;
		case JUDGE_CORE_CB_COMPILE_SUCCESS:
			data->detail_win->printf("  %s\n", _("Successful compilation."));
			break;
		case JUDGE_CORE_CB_COMPILE_FAIL:
			data->detail_win->printf("  %s\n  %s\n", _("Failed to compile."), _("Compiler output:"));
			data->detail_win->printf_color(CCOLOR_MAGENTA, "%s", data->ptr_result_prob->compiler_output.c_str());
			break;
		case JUDGE_CORE_CB_START_EXECUTE:
			data->detail_win->printf("  %s %d\n  %s\n", _("Point"), addition_info + 1,  _("Executing the program..."));
			break;
		case JUDGE_CORE_CB_FINISH_EXECUTE:
			data->detail_win->printf("  %s\n", _("The program ended."));
			break;
		case JUDGE_CORE_CB_START_CHECK:
			data->detail_win->printf("  %s\n", _("Verifing the output ..."));
			break;
		case JUDGE_CORE_CB_FINISH_ALL:
			switch ((*(data->ptr_result_point))[addition_info].exe_status)
			{
				case Judge_result_point::EXE_NORMAL :
					score = (*(data->ptr_result_point))[addition_info].score;
					if (score > 0)
						data->detail_win->printf_color(CCOLOR_GREEN, "  %s (%.3lf)\n", _("Right"), score);
					else data->detail_win->printf_color(CCOLOR_CYAN, "  %s\n", _("Wrong Answer"));
					break;
				case Judge_result_point::EXE_TIMEOUT :
					data->detail_win->printf_color(CCOLOR_RED, "  %s\n", _("Timeout"));
					break;
				case Judge_result_point::EXE_MEMORYOUT :
					data->detail_win->printf_color(CCOLOR_RED, "  %s\n", _("Memory Out"));
					break;
				case Judge_result_point::EXE_EXIT_NONZERO :
					data->detail_win->printf_color(CCOLOR_RED, "  %s\n", _("A non-zero value is returned."));
					break;
			}
			data->detail_win->printf("\n");
			break;
	}
	update();
	return false;
}

double cb_fc_verifier(double fullscore, const char *std_input_path,
		const char *std_output_path, const char *user_output_path, void *data0)
{
	Shared_data *data = static_cast<Shared_data*>(data0);
	char command[CSTRING_MAX_LEN], filename[CSTRING_MAX_LEN];
	snprintf(filename, CSTRING_MAX_LEN, "%sscore", TEMP_DIRECTORY);
	snprintf(command, CSTRING_MAX_LEN, "%s '%lf' '%s' '%s' '%s' > %s",
			data->sj_path, fullscore, std_input_path, std_output_path, user_output_path, filename);
	int ret;
	if ((ret = system(command)) != 0)
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Call to system(\"%s\") failed while running verifier. system() returned %d.\nerrno:%s\n(At %s : %s : %d)"),
				command, ret, strerror(errno), __FILE__, __PRETTY_FUNCTION__, __LINE__);
		data->sj_error = true;
		data->stop_flag = true;
		return 0;
	}
	ifstream fin(filename);
	double score;
	if (!(fin >> score))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, "%s",
				_("The customized verifier did not produce a correct output file."));
		data->sj_error = true;
		data->stop_flag = true;
		return 0;
	}
	fin.close();
	if (unlink(filename))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("unlink: %s.\n(At %s : %s : %d)"),
				strerror(errno), __FILE__, __PRETTY_FUNCTION__, __LINE__);
		data->sj_error = true;
		data->stop_flag = true;
		return 0;
	}
	return score;
}

void* thread_watch_stop(void *data0)
{
	Shared_data *data = static_cast<Shared_data*>(data0);
	while (true)
	{
		int ch = get_key();
		if (ch == 's')
			data->stop_flag = true;
	}
	return NULL;
}

bool get_contestants(Str_array &result, const Contest_config &ct_conf)
{
	result.clear();
	Str_array names_;
	if (!list_dir_in_dir(ct_conf.source_dir.c_str(), names_))
		return false;
	if (names_.size() == 0)
		return true;
	char **names = new char*[names_.size() + 1];
	int maxwidth = 0;
	for (Str_array::size_type i = 0; i < names_.size(); i ++)
	{
		names[i] = new char[names_[i].length() + 2];
		strcpy(names[i] + 1, names_[i].c_str());
		names[i][0] = ' ';
		int tmp = names_[i].length() + 5;
		if (tmp > maxwidth)
			maxwidth = tmp;
	}
	names[names_.size()] = NULL;
	bool *chosen = new bool[names_.size()];
	static const int key_allowable[] = {' ', 'a', 'c', 0};
	Window main_win;
	Menu menu;
	int ncols = main_win.get_ncols() / maxwidth;
	if (ncols < 0)
		ncols = 1;
	menu.set_arg(main_win, names, key_allowable,
			"Press SPACE to mark a contestant, 'a' to choose all, or 'c' to apply.", 0, 0, -1, -1, ncols);
	menu.multiple_choice(chosen, names);
	for (Str_array::size_type i = 0; i < names_.size(); i ++)
	{
		delete []names[i];
		if (chosen[i])
			result.push_back(names_[i]);
	}
	delete []chosen;
	delete []names;
	return true;
}

void get_problems(Iter_Problem_config_array &result, const Contest_config &ct_conf)
{
	result.clear();
	char **names = new char*[ct_conf.problems.size() + 1];
	int maxwidth = 0;
	for (Str_array::size_type i = 0; i < ct_conf.problems.size(); i ++)
	{
		const string& prob_name = ct_conf.problems[i].name;
		names[i] = new char[prob_name.length() + 2];
		strcpy(names[i] + 1, prob_name.c_str());
		names[i][0] = ' ';
		int tmp = prob_name.length() + 5;
		if (tmp > maxwidth)
			maxwidth = tmp;
	}
	names[ct_conf.problems.size()] = NULL;
	bool *chosen = new bool[ct_conf.problems.size()];
	static const int key_allowable[] = {' ', 'a', 'c', 0};
	Window main_win;
	Menu menu;
	int ncols = main_win.get_ncols() / maxwidth;
	if (ncols < 0)
		ncols = 1;
	menu.set_arg(main_win, names, key_allowable, 
			"Press SPACE to mark a problem, 'a' to choose all, or 'c' to apply.", 0, 0, -1, -1, ncols);
	menu.multiple_choice(chosen, names);
	for (Problem_config_array::size_type i = 0; i < ct_conf.problems.size(); i ++)
	{
		delete []names[i];
		if (chosen[i])
			result.push_back(ct_conf.problems.begin() + i);
	}
	delete []chosen;
	delete []names;
}

bool cb_judge_core_quiet(Judge_core_cb_info info, int addition_info, void *data0)
{
	Shared_data *data = static_cast<Shared_data*>(data0);
	return data->stop_flag;
}

