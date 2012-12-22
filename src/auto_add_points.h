/*
 * $File: auto_add_points.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Fri Sep 25 22:57:31 2009
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

#ifndef HEADER_AUTO_ADD_POINTS
#define HEADER_AUTO_ADD_POINTS

#include <vector>
#include <string>
struct Point_info
{
	std::string input, output;
};

typedef std::vector<Point_info> Point_info_array;

bool auto_add_points(const char *data_dir, Point_info_array &result); 
	// return ture on success or false if failed
	// if global_error_msg is non-empty, it will indicate the system error.
#endif

