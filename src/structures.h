/*
 * $File: structures.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Thu Sep 24 23:31:34 2009
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

#ifndef HEADER_STRUCTURES
#define HEADER_STRUCTURES

#include "conf.h"

#include <vector>
#include <string>
#include <map>

typedef std::vector<std::string> Str_array;

struct Problem_config;
struct Test_point
{
	int time;  // measured by seconds
	long rlimit_as, rlimit_data, rlimit_stack;
	double score;
	std::string input, output;
	Test_point& operator = (const Test_point &a);
private:
	friend struct Problem_config;
	bool init_v0(const Conf &conf, const char *section, int num);
	bool init_v1(const Conf &conf, const char *section, int num);
	void write_conf(Conf &conf, const char *section, int num) const;
};
typedef std::vector<Test_point> Test_point_array;

struct Contest_config;
struct Problem_config
{
	std::string name, input, output, source, compare_func;
	// @compare_func:
	//		"s"		standard
	//		"u%s"	user defined, whose path is %s
	Test_point_array points;
	void init(const char *probname);  // construct a totally new problem
private:
	friend struct Contest_config;
	bool init_v0(const Conf &conf, int prob_num);
	bool init_v1(const Conf &conf, int prob_num);
	void write_conf(Conf &conf, int num) const;
};
typedef std::vector<Problem_config> Problem_config_array;

typedef std::map<std::string, std::string> Str_map;
struct Contest_config
{
	Str_map compiler;
	std::string source_dir, contest_name;
	// @source_dir should end with '/'
	Problem_config_array problems;
	bool init(const Conf &conf);
	bool init_v0(const Conf &conf);
	bool init_v1(const Conf &conf);
	void write_conf(Conf &conf) const;
};

// ===================================================================== 

struct Judge_result_point
{
	double score;
	unsigned long time, memory;
	// @time is measured by 1e-6 seconds ; @memory is measured by kb 
	enum Execute_status {EXE_NORMAL, EXE_TIMEOUT, EXE_MEMORYOUT, EXE_EXIT_NONZERO};
	Execute_status exe_status;
};
typedef std::vector<Judge_result_point> Judge_result_point_array;

struct Judge_result_contestant;
struct Judge_result_problem
{
	bool source_found, compile_successful;
	std::string compiler_output;
	Judge_result_point_array points;

	bool convert2str(std::string &result) const;
	bool init(const std::string &str);
		// return false if failed.
};
#define JUDGE_RESULT_CONF_VERSION	"1"

#endif // HEADER_STRUCTURES

