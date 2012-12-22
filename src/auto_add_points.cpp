/*
 * $File: auto_add_points.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Fri Sep 25 23:14:37 2009
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <cstdio>
#include <cstring>
#include <algorithm>

#include "auto_add_points.h"
#include "common.h"
using namespace std;

typedef vector<string> Str_array;

//-----------------------Function Declaration--------------------------
static bool list_dir(const char *dir, Str_array &files); //it only lists regular files
static void adjust_io(Point_info &pi, const Point_info &lower);
static void sort(Str_array &array, Str_array &extra_info);
/*
0.
	%common%-%var%-%id_in%
	%common%-%var%-%id_out%
1.
	%common%-%id_in%-%var%-%common%
	%common%-%id_out%-%var%-%common%
*/
static int judge_type(const Str_array &array);
static int get_common_left(const string &str1, const string &str2);
static bool str_right_cmp(const string &right, const string &str);
//---------------------------------------------------------------------

bool auto_add_points(const char *dir, Point_info_array &ans)
{
	global_error_msg[0] = 0;
	ans.clear();
	Str_array files;
	if (!list_dir(dir, files) || files.size() % 2 || files.size() <= 4)
		return false;
	Str_array files_orig;
	for (Str_array::iterator i = files.begin(); i != files.end(); i ++)
	{
		files_orig.push_back(*i);
		for (string::iterator j = i->begin(); j != i->end(); j ++)
			if (*j >= 'A' && *j <= 'Z')
				*j += 'a' - 'A';
	}
	ans.resize(files.size() / 2);
	sort(files, files_orig);
	Point_info_array ans_lowercase;
	ans_lowercase.resize(ans.size());
	if (judge_type(files) == 0)
	{
		for (Str_array::size_type i = 0; i < ans.size(); i ++)
		{
			ans[i].input = files_orig[i * 2];
			ans[i].output = files_orig[i * 2 + 1];
			ans_lowercase[i].input = files[i * 2];
			ans_lowercase[i].output = files[i * 2 + 1];
		}
	} else
	{
		for (Str_array::size_type i = 0; i < ans.size(); i ++)
		{
			ans[i].input = files_orig[i];
			ans[i].output = files_orig[i + ans.size()];
			ans_lowercase[i].input = files[i];
			ans_lowercase[i].output = files[i + ans.size()];
		}
	}
	for (Str_array::size_type i = 0; i < ans.size(); i ++)
		adjust_io(ans[i], ans_lowercase[i]);
	return true;
}

int judge_type(const Str_array &array)
{
	int maxlen = get_common_left(array[0], array[1]);
	string id_in(array[0].substr(maxlen)), id_out(array[1].substr(maxlen));
	if (str_right_cmp(id_in, array[2]) && str_right_cmp(id_out, array[3]))
		return 0;
	return 1;
}

bool str_right_cmp(const string &right, const string &str)
{
	if (right.length() > str.length())
		return false;
	return right == str.c_str() + str.length() - right.length();
}

int get_common_left(const string &str1, const string &str2)
{
	string::const_iterator p1 = str1.begin(), p2 = str2.begin();
	int ans = 0;
	while (p1 != str1.end() && p2 != str2.end() && *p1 == *p2)
	{
		ans ++;
		p1 ++;
		p2 ++;
	}
	return ans;
}

void sort(Str_array &array, Str_array &extra_info)
{
	for (Str_array::size_type i = 0; i < array.size(); i ++)
		for (int j = array.size() - 2; j >= int(i); j --)
			if (array[j] > array[j + 1])
			{
				swap(array[j], array[j + 1]);
				swap(extra_info[j], extra_info[j + 1]);
			}
}

bool list_dir(const char *dirpath, Str_array &files)
{
	files.clear();
	if (chdir(dirpath))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("%s : %s : %d:  chdir(\"%s\"): %s\n"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__, dirpath, strerror(errno));
		return false;
	}

	DIR *dir = opendir(".");
	if (dir == NULL)
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("%s : %s : %d:  opendir: %s\n"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__, strerror(errno));
		return false;
	}
	dirent *pdirent;
	while ((pdirent = readdir(dir)) != NULL)
	{
		if (pdirent->d_name[0] == '.')
			continue;
		struct stat buf;
		if (stat(pdirent->d_name, &buf))
		{
			snprintf(global_error_msg, CSTRING_MAX_LEN, _("%s : %s : %d:  stat: %s\n"),
					__FILE__, __PRETTY_FUNCTION__, __LINE__, strerror(errno));
			return false;
		}
		if (S_ISREG(buf.st_mode))
		{
			files.push_back(string());
			files.rbegin()->assign(pdirent->d_name);
		}
	}
	closedir(dir);
	if (fchdir(fd_config_work_dir))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("%s : %s : %d:  fchdir: %s\n"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__, strerror(errno));
		return false;
	}
	return true;
}

void adjust_io(Point_info &pi, const Point_info &lower)
{
	int pos = lower.input.length() - 1;
	while (pos >= 0 && pi.input[pos] != '.')
		pos --;
	string tmp = pi.input.substr(pos + 1);
	if (tmp.find("in", 0) != string::npos)
		return;
	if (tmp.find("ou", 0) != string::npos || tmp.find("ans", 0) != string::npos)
	{
		swap(pi.input, pi.output);
		return;
	}
	if (lower.input.find("ou", 0) != string::npos)
		swap(pi.input, pi.output);
}

