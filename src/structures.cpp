/*
 * $File: structures.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Jul 06 11:05:14 2011 +0800
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

#include "structures.h"
#include "common.h"
#include "conf.h"
#include "bstream.h"
using namespace std;

#include <cstdio>

bool Contest_config::init(const Conf &conf)
{
	string version;
	if (!conf.read("general", "VERSION", version))
		return init_v0(conf);
	else if (version == "1")
		return init_v1(conf);
	snprintf(global_error_msg, CSTRING_MAX_LEN, _("Invalid version of the configuration file."));
	return false;
}

bool Contest_config::init_v0(const Conf &conf)
{
	if (conf.list_section_start("compiler.command"))
	{
		for (Conf::Section_item *ptr = conf.list_section_next(); ptr != NULL; ptr = conf.list_section_next())
			compiler[ptr->item] = ptr->value;
	}else 
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch the 'compiler.command' section.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	if (!conf.read("general", "src", source_dir))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch source file directory.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	if (*source_dir.rbegin() != '/')
		source_dir.append("/");
	if (!conf.read("general", "name", contest_name))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch contest name.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	int nproblems;
	if (!conf.read("general", "problems_num", nproblems))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch number of problems.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	problems.resize(nproblems);
	for (int i = 0; i < nproblems; i ++)
		if (!problems[i].init_v0(conf, i))
			return false;
	return true;
}

bool Contest_config::init_v1(const Conf &conf)
{
	if (conf.list_section_start("compiler"))
	{
		for (Conf::Section_item *ptr = conf.list_section_next(); ptr != NULL; ptr = conf.list_section_next())
			compiler[ptr->item] = ptr->value;
	}else 
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch the 'compiler' section.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	if (!conf.read("general", "src", source_dir))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch source file directory.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	if (*source_dir.rbegin() != '/')
		source_dir.append("/");
	if (!conf.read("general", "name", contest_name))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch contest name.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	int nproblems;
	if (!conf.read("general", "problems_num", nproblems))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, 
				_("Unable to fetch number of problems.\n(At %s : %s : %d)"),
				__FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	problems.resize(nproblems);
	for (int i = 0; i < nproblems; i ++)
		if (!problems[i].init_v1(conf, i))
			return false;
	return true;
}

void Problem_config::init(const char *probname)
{
	name = probname;
	input = probname;
	input.append(".in");
	output = probname;
	output.append(".out");
	source = probname;
	compare_func = "s";
	points.clear();
}

bool Problem_config::init_v0(const Conf &conf, int prob_num)
{
	char tmp[CSTRING_MAX_LEN];
	snprintf(tmp, CSTRING_MAX_LEN, "problem_%d_name", prob_num);
	if (!conf.read("general", tmp, name))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Unable to fetch the name of problem #%d.\n(At %s : %s : %d)"), 
				prob_num, __FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	snprintf(tmp, CSTRING_MAX_LEN, "%s.general", name.c_str());
#define OPERATE(_x_) \
	if (!conf.read(tmp, # _x_, _x_)) \
	{\
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Unable to fetch the '%s' field of problem #%d.\n(At %s : %s : %d)"),  \
				# _x_, prob_num, __FILE__, __PRETTY_FUNCTION__, __LINE__); \
		return false; \
	}
	OPERATE(input);
	OPERATE(output);
	OPERATE(source);
#undef OPERATE
	compare_func = "s";
	int npoints;
	if (!conf.read(tmp, "points_num", npoints))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Unable to fetch the number of points of problem #%d.\n(At %s : %s : %d)"), 
				prob_num, __FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	snprintf(tmp, CSTRING_MAX_LEN, "%s.points", name.c_str());
	points.resize(npoints);
	for (int i = 0; i < npoints; i ++)
		if (!points[i].init_v0(conf, tmp, i))
			return false;
	return true;
}

bool Problem_config::init_v1(const Conf &conf, int prob_num)
{
	char tmp[CSTRING_MAX_LEN];
	snprintf(tmp, CSTRING_MAX_LEN, "%d.general", prob_num);
#define OPERATE(_x_) \
	if (!conf.read(tmp, # _x_, _x_)) \
	{\
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Unable to fetch the '%s' field of problem #%d.\n(At %s : %s : %d)"),  \
				# _x_, prob_num, __FILE__, __PRETTY_FUNCTION__, __LINE__); \
		return false; \
	}
	OPERATE(name);
	OPERATE(input);
	OPERATE(output);
	OPERATE(source);
	OPERATE(compare_func);
#undef OPERATE
	if (compare_func.empty() || (compare_func[0] != 'u' && compare_func != "s"))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Invalid verifier '%s' of problem #%d.\n(At %s : %s : %d)"),  \
				compare_func.c_str(), prob_num, __FILE__, __PRETTY_FUNCTION__, __LINE__); \
		return false; \
	}
	int npoints;
	if (!conf.read(tmp, "points_num", npoints))
	{
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Unable to fetch the number of points of problem #%d.\n(At %s : %s : %d)"), 
				prob_num, __FILE__, __PRETTY_FUNCTION__, __LINE__);
		return false;
	}
	snprintf(tmp, CSTRING_MAX_LEN, "%d.points", prob_num);
	points.resize(npoints);
	for (int i = 0; i < npoints; i ++)
		if (!points[i].init_v1(conf, tmp, i))
			return false;
	return true;
}

#define OPERATE(_x_) \
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.%s", num, # _x_); \
	if (!conf.read(section, conf_item, _x_)) \
	{ \
		snprintf(global_error_msg, CSTRING_MAX_LEN, _("Unable to fetch the '%s' item in section '%s'.\n(At %s : %s : %d)"),  \
				conf_item, section, __FILE__, __PRETTY_FUNCTION__, __LINE__); \
		return false; \
	}
bool Test_point::init_v0(const Conf &conf, const char *section, int num)
{
	char conf_item[CSTRING_MAX_LEN];
	OPERATE(time);
	OPERATE(score);
	OPERATE(input);
	OPERATE(output);
	rlimit_as = -1;
	rlimit_data = -1;
	rlimit_stack = -1;
	return true;
}

bool Test_point::init_v1(const Conf &conf, const char *section, int num)
{
	char conf_item[CSTRING_MAX_LEN];
	OPERATE(time);
	OPERATE(score);
	OPERATE(input);
	OPERATE(output);
	OPERATE(rlimit_as);
	OPERATE(rlimit_data);
	OPERATE(rlimit_stack);
	return true;
}
#undef OPERATE

Test_point& Test_point::operator = (const Test_point &a)
{
	time = a.time;
	rlimit_as = a.rlimit_as;
	rlimit_data = a.rlimit_data;
	rlimit_stack = a.rlimit_stack;
	score = a.score;
	input = a.input;
	output = a.output;
	return *this;
}

void Contest_config::write_conf(Conf &conf) const
{
	conf.del_all();
	conf.write("general", "name", contest_name);
	conf.write("general", "src", source_dir);
	conf.write("general", "problems_num", (int)problems.size());
	conf.write("general", "VERSION", "1");
	for (Problem_config_array::size_type i = 0; i != problems.size(); i ++)
		problems[i].write_conf(conf, i);
	for (Str_map::const_iterator i = compiler.begin(); i != compiler.end(); i ++)
		conf.write("compiler", i->first.c_str(), i->second);
}

void Problem_config::write_conf(Conf &conf, int num) const
{
	char conf_section[CSTRING_MAX_LEN];
	snprintf(conf_section, CSTRING_MAX_LEN, "%d.general", num);
	conf.write(conf_section, "name", name);
	conf.write(conf_section, "input", input);
	conf.write(conf_section, "output", output);
	conf.write(conf_section, "source", source);
	conf.write(conf_section, "compare_func", compare_func);
	conf.write(conf_section, "points_num", int(points.size()));
	snprintf(conf_section, CSTRING_MAX_LEN, "%d.points", num);
	for (Test_point_array::size_type i = 0; i < points.size(); i ++)
		points[i].write_conf(conf, conf_section, i);
}

void Test_point::write_conf(Conf &conf, const char *section, int num) const
{
	char conf_item[CSTRING_MAX_LEN];
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.time", num);
	conf.write(section, conf_item, time);
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.score", num);
	conf.write(section, conf_item, score);
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.input", num);
	conf.write(section, conf_item, input);
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.output", num);
	conf.write(section, conf_item, output);
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.rlimit_as", num);
	conf.write(section, conf_item, rlimit_as);
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.rlimit_data", num);
	conf.write(section, conf_item, rlimit_data);
	snprintf(conf_item, CSTRING_MAX_LEN, "%d.rlimit_stack", num);
	conf.write(section, conf_item, rlimit_stack);
}

// format:
//   "%b%b%b%s%s%d%x...%x", source_found, compile_successful, compiler_output, points.size(), points[0], points[1], ...
//   %x: "%lf%ld%ld%d", score, time, memory, exe_status

bool Judge_result_problem::convert2str(string &result) const
{
	Obstream os;
	os << source_found << compile_successful << compiler_output << (int)points.size();
	for (Judge_result_point_array::const_iterator iter = points.begin(); iter != points.end(); iter ++)
		os << iter->score << iter->time << iter->memory << (int)(iter->exe_status);

	if (os.str() == NULL)
		return false;

	result.assign(os.str());
	return true;
}

bool Judge_result_problem::init(const string &str)
{
	try
	{
		Ibstream is(str.c_str());
		int npoint;
		is >> source_found >> compile_successful >> compiler_output >> npoint;
		points.resize(npoint);
		for (Judge_result_point_array::iterator iter = points.begin(); iter != points.end(); iter ++)
		{
			int tmp;
			is >> iter->score >> iter->time >> iter->memory >> tmp;
			iter->exe_status = (Judge_result_point::Execute_status)tmp;
		}
		return true;
	}catch (Ibstream::Error)
	{
		return false;
	}
}

