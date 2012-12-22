/*
 * $File: conf.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sun Sep 20 12:21:52 2009
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

#ifndef HEADER_CONF
#define HEADER_CONF

#include <string>

class Conf
{
	static const int INTEGER_LENGTH = 20, DOUBLE_LENGTH  = 100;

	struct List_section;
	struct List_item;
	List_section *datahead, *datatail;

	mutable List_item *list_section_pos;

	char *file_path;

	void init();

	List_section* lookfor_section(const char *name) const; // return NULL if nothing is found.
	List_item* lookfor_item(const List_section *section, const char *name) const; //return NULL if nothing is found.

	Conf(const Conf &) {}
public:
	Conf(const char *file);

	bool empty() const; // whether there is nothing in the configuration

	bool read(const char *section, const char *item, double &value) const;
	bool read(const char *section, const char *item, int &value) const;
	bool read(const char *section, const char *item, long &value) const {int tmp; if (!read(section, item, tmp)) return false; value = tmp; return true;}
	bool read(const char *section, const char *item, char *value, unsigned maxlen) const; //maxlen includes the '\0'.
	bool read(const char *section, const char *item, std::string &value) const;

	void write(const char *section, const char *item, double value);
	void write(const char *section, const char *item, int value);
	void write(const char *section, const char *item, long value) {write(section, item, (int)value);}
	void write(const char *section, const char *item, const char *value);
	void write(const char *section, const char *item, const std::string &value);

	void delitem(const char *section, const char *item);
	void delsection(const char *section);
	void del_all();

	bool save() const;
	bool save_as(const char *file_path_new);

	unsigned long int get_item_length(const char *section, const char *item) const;

	const char* get_file_path() const;
	
	struct Section_item
	{
		const char *item, *value;
	};
	bool list_section_start(const char *section) const ;  // return false if the section does not exist
	Section_item *list_section_next() const ;  // return NULL if reached the end

	~Conf();
};

#endif //HEADER_CONF

