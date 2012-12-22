/*
 * $File: filecmp.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Jul 06 10:24:00 2011 +0800
 */
/*
This file is part of injuslin, informatcis judge system for linux.

Copyright (C) <2011>  Jiakai <jia.kai66@gmail.com>

Injuslin is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Injuslin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with injuslin.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _HEADER_FILECMP_
#define _HEADER_FILECMP_
extern double filecmp_normal(double fullscore, const char *std_input_path,
		const char *std_output_path, const char *user_output_path, void *data);
#endif

