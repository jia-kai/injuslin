/*
 * $File: view_results.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Fri Sep 25 18:47:30 2009
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

#ifndef HEADER_VIEW_RESULTS
#define HEADER_VIEW_RESULTS

void view_results(const char *conf_path);
void export_results2html(const char *conf_path, const char *file_path);
void export_results2stdout(const char *conf_path, const char *contestant = NULL);

#endif


