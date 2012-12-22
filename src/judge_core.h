/*
 * $File: judge_core.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sat Dec 22 11:14:06 2012 +0800
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

#ifndef HEADER_JUDGE_CORE
#define HEADER_JUDGE_CORE

#include "conf.h"
#include "structures.h"
#include <stdint.h>

typedef double (*File_compare)(double fullmark, const char *std_input_path, 
		const char *std_output_path, const char *user_output_path, void *user_data);

enum Judge_core_cb_info
{
	JUDGE_CORE_CB_SRC_FOUND, // @addition_info will be a c-format const string (i.e. (int)var, where var is of type const char *)
	JUDGE_CORE_CB_SRC_NOT_FOUND,
	JUDGE_CORE_CB_START_COMPILE,
	JUDGE_CORE_CB_COMPILE_SUCCESS,
	JUDGE_CORE_CB_COMPILE_FAIL,

	JUDGE_CORE_CB_START_EXECUTE,
	JUDGE_CORE_CB_FINISH_EXECUTE,
	JUDGE_CORE_CB_START_CHECK,

	JUDGE_CORE_CB_FINISH_ALL
		// for JUDGE_CORE_CB_START_EXECUTE, JUDGE_CORE_CB_FINISH_EXECUTE
		//     JUDGE_CORE_CB_START_CHECK and JUDGE_CORE_CB_FINISH_ALL:
		//     @addition_info will be the number of test point being judged, starting at 0
};
typedef bool (*Judge_core_cb)(Judge_core_cb_info info, size_t addition_info, void *user_data); 
	// return true to stop the judge process

// Func:  judge_core  --  judge a contestant's solution for a problem
//  @ct_conf	:	the contest configuration, containing compiler and source_dir
//		used fields:  source_dir, compiler
//  @prob_conf	:	the configuration of the problem
//  @fc			:	function to compare the standard output and the user's output, returning the score
//  @result		:	the judge result
//  @cb_result_updated	:	
//		If it is not NULL, the function pointed by @cb_result_updated will be called when new progress is done
// Return		:
//		On success, true is returned. 
//		If a system error occurs, return false and the global variable @global_error_msg in common.h is set.
bool judge_core(const Contest_config &ct_conf, const Problem_config &prob_conf, const std::string &contestant_name,
		File_compare fc, void *fc_user_data, Judge_result_problem &result, Judge_core_cb cb_result_updated = NULL, void *cb_user_data = NULL);

#endif // HEADER_JUDGE_CORE
