/*
 * $File: common.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sat Jul 16 19:40:09 2011 +0800
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
#ifndef HEADER_COMMON
#define HEADER_COMMON

#ifndef EXECUTE_WITH_CHROOT
#define TEMP_DIRECTORY		"/tmp/injuslin_temp/"
#else
#define EXECUTE_ROOT_DIR	"/judge"
#define TEMP_DIRECTORY		"/judge/tmp/injuslin/"
#define EXECUTE_CHDIR		"/tmp/injuslin"
	// EXECUTE_CHDIR is based on EXECUTE_ROOT_DIR
#endif

/*
 * Note:
 *
 *    TEMP_DIRECTORY  _MUST_ end with a slash
 *
 *    injuslin must have the wr permition of the parent directory of TEMP_DIRECTORY.
 *
 *
 *    If you are sure to use chroot environment, at first, you MUST be root to run injuslin,
 *      and either of the following two thing must be done:
 *
 *         1. Add "-static" option to gcc/g++ (quotes not included)
 *         2. Copy necessary libraries to correct directory.
 *            You can simpily do this by executing init_chroot.sh
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#define	VERSION_INF		PACKAGE_STRING
#else
#define	VERSION_INF		"injuslin --unknown version"
#undef ENABLE_NLS
#endif

#ifdef _
#undef _
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(_s_) (gettext((_s_)))
#else
#define _(_s_) (_s_)
#endif

#define AUTHOR				"Jiakai <jia.kai66@gmail.com>"
#define CSTRING_MAX_LEN		2048
#define CSTR_INT_MAX_LEN	11
#define CSTR_NUM_MAX_LEN	50

#define EXECUTE_USER		"nobody"

#ifdef DEBUG
void write_debug_msg(const char *msg);
void write_debug_msg(int msg);
#endif

#ifdef NEED_LIST_DIR
#include <string>
#include <vector>
bool list_dir_in_dir(const char *path, std::vector<std::string> &ans);
	//  only lists directorys. 
	//  return true if it is successful.
#endif

extern char global_error_msg[CSTRING_MAX_LEN];

extern int fd_orig_work_dir, fd_config_work_dir;
	// initialized in main()


#define ITER_VECTOR(v, var) \
	for (typeof((v).begin()) var = (v).begin(); var != (v).end(); var ++)

#define ITER_VECTOR_IDX(v, var) \
	for (typeof((v).size()) var = 0; var < (v).size(); var ++)

#endif // HEADER_COMMON
