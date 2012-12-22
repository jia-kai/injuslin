/*
 * $File: common.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Fri Sep 25 21:03:11 2009
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

#define NEED_LIST_DIR
#include "common.h"
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include <cstring>
#include <cstdio>

char global_error_msg[CSTRING_MAX_LEN];
int fd_orig_work_dir, fd_config_work_dir;

bool list_dir_in_dir(const char *dirpath, vector<string> &ans)
{
#define RETURN(value) \
	do \
	{ \
		if (dir != NULL) \
			closedir(dir); \
		if (fchdir(fd_config_work_dir)) \
			return false; \
		return (value); \
	} while(0)
	ans.clear();
	if (chdir(dirpath))
		return false;
	DIR *dir = opendir(".");
	if (dir == NULL)
		RETURN(false);
	dirent *pdirent;
	struct stat stat_info;
	while ((pdirent = readdir(dir)) != NULL)
	{
		if (pdirent->d_name[0] == '.')
			continue;
		if (stat(pdirent->d_name, &stat_info))
			RETURN(false);
		if (S_ISDIR(stat_info.st_mode))
			ans.push_back(string(pdirent->d_name));
	}
	RETURN(true);
#undef RETURN
}

#ifdef DEBUG
void write_debug_msg(const char *msg)
{
	static int count = 0;
	FILE *fout = fopen("debug.log", "a");
	fprintf(fout, "[string %d]%s \n", count ++, msg);
	fclose(fout);
}

void write_debug_msg(int msg)
{
	static int count = 0;
	FILE *fout = fopen("debug.log", "a");
	fprintf(fout, "[int %d]%d \n", count ++, msg);
	fclose(fout);
}
#endif // DEBUG

