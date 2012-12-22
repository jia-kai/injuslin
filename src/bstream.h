/*
 * $File: bstream.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sun Sep 20 12:21:34 2009
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
#ifndef HEADER_BSTREAM
#define HEADER_BSTREAM

#include <string>

class Ibstream
{
	struct Detail;
	Detail *p;
	Ibstream (const Ibstream &) {}
public:
	struct Error {}; // any reading operation may throw Error()
	Ibstream(const char *str);
	~Ibstream();
	Ibstream& operator >> (bool &x);
	Ibstream& operator >> (int &x);
	Ibstream& operator >> (unsigned long &x);
	Ibstream& operator >> (double &x);
	Ibstream& operator >> (std::string &x);
};

class Obstream
{
	struct Detail;
	Detail *p;
	Obstream(const Obstream &) {}
public:
	Obstream();
	~Obstream();
	const char *str(); // return NULL if failed to calculate the string.
	Obstream& operator << (const bool &x);
	Obstream& operator << (const int &x);
	Obstream& operator << (const unsigned long &x);
	Obstream& operator << (const double &x);
	Obstream& operator << (const std::string &x);
};

#endif
