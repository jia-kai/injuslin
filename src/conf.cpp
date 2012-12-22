/*
 * $File: conf.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Fri Oct 16 00:53:52 2009
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

#include "conf.h"
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
using namespace std;

//-------------Function Declaration---------------
static bool readline(ifstream &fin, string &line); //read the next legitimate line in "fin".
static void translateline(const string &line, string &name, string &value); //translate a string like "a=b".
//------------------------------------------------

struct Conf::List_section
{
	List_item *itemhead, *itemtail;
	List_section *next;
	string name;
	List_section(const string &name_) : itemhead(NULL), itemtail(NULL), next(NULL), name(name_) {}
};

struct Conf::List_item
{
	List_item *next;
	string name, value;
	List_item(const string &name_, const string &value_) : next(NULL), name(name_), value(value_) {}
};

Conf::Conf(const char *file) :  datahead(NULL), datatail(NULL), list_section_pos(NULL)
{
	file_path = new char[strlen(file) + 1];
	strcpy(file_path, file);
	init();
}

Conf::~Conf()
{
	del_all();
	delete []file_path;
}

void Conf::init()
{
	ifstream fin(file_path);
	string tmp;
	if (!readline(fin, tmp))
		return;
	if (tmp[0] == '[')
		datahead = new List_section(tmp.substr(1, tmp.length() - 2));
	else datahead = new List_section("");
	List_section *ps = datahead;
	while (fin)
	{
		if (!readline(fin, tmp))
			break;
		if (tmp[0] == '[')
		{
			ps->next = new List_section(tmp.substr(1, tmp.length() - 2));
			ps = ps->next;
			continue;
		}
		string name, value;
		translateline(tmp, name, value);
		if (ps->itemhead == NULL)
		{
			ps->itemhead = new List_item(name, value);
			ps->itemtail = ps->itemhead;
			continue;
		}
		ps->itemtail->next = new List_item(name, value);
		ps->itemtail = ps->itemtail->next;
	}
	datatail = ps;
}

bool readline(ifstream &fin, string &line)
{
	while (fin)
	{
		getline(fin, line);
		if (line.empty())
			continue;
		if (line[0] == '[')
		{
			if (*line.rbegin() != ']')
				continue;
			return true;
		}
		if (line[0] != '=' && !isalpha(line[0]) && !isdigit(line[0]))
			continue;
		for (string::iterator i = line.begin(); i != line.end(); i ++)
			if (*i == '=')
				return true;
	}
	return false;
}

void translateline(const string &line, string &name, string &value)
{
	string::size_type loc = line.find('=', 0);
	name = line.substr(0, loc);
	value = line.substr(loc + 1);
}

bool Conf::read(const char *section, const char *item, double &value) const
{
	char ret[DOUBLE_LENGTH + 1];
	if (!read(section, item, ret, DOUBLE_LENGTH))
		return false;
	return sscanf(ret, "%le", &value) == 1;
}

bool Conf::read(const char *section, const char *item, int &value) const
{
	char ret[INTEGER_LENGTH + 1];
	if (!read(section, item, ret, INTEGER_LENGTH))
		return false;
	return sscanf(ret, "%d", &value) == 1;
}

bool Conf::read(const char *section, const char *item, char *value, unsigned maxlen) const
{
	List_item *pi = lookfor_item(lookfor_section(section), item);
	if (pi == NULL)
		return false;
	if (pi->value.length() >= maxlen)
		return false;
	strcpy(value, pi->value.c_str());
	return true;
}

bool Conf::read(const char *section, const char *item, std::string &value) const
{
	List_item *pi = lookfor_item(lookfor_section(section), item);
	if (pi == NULL)
		return false;
	value = pi->value;
	return true;
}

Conf::List_section* Conf::lookfor_section(const char *name) const
{
	List_section *ps = datahead;
	string tmp(name);
	while (ps != NULL)
	{
		if (ps->name == tmp)
			return ps;
		ps = ps->next;
	}
	return NULL;
}

Conf::List_item* Conf::lookfor_item(const List_section *section, const char *name) const
{
	if (section == NULL)
		return NULL;
	List_item *pi = section->itemhead;
	string tmp(name);
	while (pi != NULL)
	{
		if (pi->name == tmp)
			return pi;
		pi = pi->next;
	}
	return NULL;
}

void Conf::write(const char *section, const char *item, const char *value)
{
	if (datahead == NULL)
	{
		datahead = new List_section(section);
		datatail = datahead;
		datahead->itemhead = new List_item(item, value);
		datahead->itemtail = datahead->itemhead;
		return;
	}
	List_section *ps = lookfor_section(section);
	if (ps == NULL)
	{
		datatail->next = new List_section(section);
		datatail = datatail->next;
		datatail->itemhead = new List_item(item, value);
		datatail->itemtail = datatail->itemhead;
		return;
	}
	List_item *pi = lookfor_item(ps, item);
	if (pi == NULL)
	{
		ps->itemtail->next = new List_item(item, value);
		ps->itemtail = ps->itemtail->next;
		return;
	}
	pi->value = value;
}

void Conf::write(const char *section, const char *item, int value)
{
	char tmp[INTEGER_LENGTH];
	sprintf(tmp, "%d", value);
	write(section, item, tmp);
}

void Conf::write(const char *section, const char *item, double value)
{
	char tmp[DOUBLE_LENGTH];
	sprintf(tmp, "%le", value);
	write(section, item, tmp);
}

void Conf::write(const char *section, const char *item, const std::string &value)
{
	write(section, item, value.c_str());
}

bool Conf::save() const
{
	FILE *fout = fopen(file_path, "w");
	if (fout == NULL)
		return false;
	List_section *ps = datahead;
	while (ps != NULL)
	{
		fprintf(fout, "[%s]\n", ps->name.c_str());
		List_item *pi = ps->itemhead;
		while (pi != NULL)
		{
			fprintf(fout, "%s=%s\n", pi->name.c_str(), pi->value.c_str());
			pi = pi->next;
		}
		fprintf(fout, "\n");
		ps = ps->next;
	}
	return fclose(fout) == 0;
}

bool Conf::save_as(const char *file_path_new)
{
	delete []file_path;
	file_path = new char[strlen(file_path_new) + 1];
	strcpy(file_path, file_path_new);
	return save();
}

void Conf::delitem(const char *section, const char *item)
{
	List_section *ps = lookfor_section(section);
	if (ps != NULL)
	{
		List_item *pi_prev = NULL, *pi = ps->itemhead;
		while (pi != NULL && pi->name != item)
		{
			pi_prev = pi;
			pi = pi->next;
		}
		if (pi != NULL)
		{
			if (pi_prev == NULL && pi->next == NULL)
			{
				delsection(section);
				return;
			}
			if (pi_prev == NULL)
				ps->itemhead = pi->next;
			else pi_prev->next = pi->next;
			if (pi->next == NULL)
				ps->itemtail = pi_prev;
			delete pi;
		}
	}
}

void Conf::delsection(const char *section)
{
	List_section *ps = datahead, *ps_prev = NULL;
	while (ps != NULL && ps->name != section)
	{
		ps_prev = ps;
		ps = ps->next;
	}
	if (ps != NULL)
	{
		if (ps_prev == NULL)
			datahead = ps->next;
		else ps_prev->next = ps->next;
		if (ps->next == NULL)
			datatail = ps_prev;
		List_item *pi = ps->itemhead;
		while (pi != NULL)
		{
			List_item *tmp = pi->next;
			delete pi;
			pi = tmp;
		}
		delete ps;
	}
}

unsigned long int Conf::get_item_length(const char *section, const char *item) const
{
	List_item *pi = lookfor_item(lookfor_section(section), item);
	if (pi == NULL)
		return 0;
	return pi->value.length();
}

void Conf::del_all()
{
	List_section *ps = datahead;
	while (ps != NULL)
	{
		List_item *pi = ps->itemhead;
		while (pi != NULL)
		{
			List_item *tmp = pi->next;
			delete pi;
			pi = tmp;
		}
		List_section *tmp = ps->next;
		delete ps;
		ps = tmp;
	}
	datahead = datatail = NULL;
}

const char* Conf::get_file_path() const
{
	return file_path;
}

bool Conf::list_section_start(const char *section) const
{
	List_section *tmp;
	if ((tmp = lookfor_section(section)) == NULL)
		return false;
	list_section_pos = tmp->itemhead;
	return true;
}

Conf::Section_item* Conf::list_section_next() const
{
	if (list_section_pos == NULL)
		return NULL;
	static Section_item ret;
	ret.item = list_section_pos->name.c_str();
	ret.value = list_section_pos->value.c_str();
	list_section_pos = list_section_pos->next;
	return &ret;
}

bool Conf::empty() const
{
	return datahead == NULL;
}

