/*
 * $File: cui.h
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Sep 16 22:56:42 2009
*/
/*
Copyright (C) (2008) (Jiakai) <jia.kai66@gmail.com>

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

#ifndef HEADER_CUI_INTERFACE
#define HEADER_CUI_INTERFACE

namespace cui
{
	enum Cui_color{CCOLOR_BLACK = 1, CCOLOR_RED, CCOLOR_GREEN, CCOLOR_YELLOW, CCOLOR_BLUE, CCOLOR_MAGENTA, CCOLOR_CYAN, CCOLOR_WHITE};
	const int ENTER_KEY_CODE = 10;
	void start_cui();
	void end_cui();

	void update();
		// update the virtual screen and the physical screen.

	int get_str_nlines(const char *str, int cols = 0);
		// cols == 0 means the width of the screen

	int message_box(const char *msg, const char *title = 0);
		//  return: 
		//     the key pressed.

	int get_ncols();
	int get_nlines();
	int get_key();

	class Window;
	class Menu;
	class Menu
	{
		struct Vars;
		Vars *v;
	public:
		Menu();
		~Menu();
		void set_arg(Window &win, const char *const items[], const int *key_allowable, const char *msg = 0, 
				int height = 0, int width = 0, int starty = -1, int startx = -1, int ncols = 1);
			//  the menu's position is based on @win.
			//  if @height or @width is equal to 0, the height or width of @win will be assigned to it.
			//  if @startx or @starty is equal to -1, the menu which is @width wide and @height high will be put in the middle of @win.
			//  @items and @key_allowable should end with a value of 0
		int wait_key(); 
			// wait until a key in @key_allowable is pressed. 
			// return value is the key pressed.
		bool wait_move_key(int &key_pressed);
			// wait until a key in @key_allowable is pressed or the current item is changed.
			// return value is true if the current item is changed , or false if a key in @key_allowable is pressed.
		int get_choice();
		int multiple_choice(bool *result, char *items[]);
			// @result should be able to hold as many bools as the number of items. 
			// @key_allowable:
			//   [0] - choose one ; [1] - choose all ; [2..n] - return
			// @items should be the arg @items passed in set_arg, and begin with a space.
			// return value is the key pressed.
		void free();
	};
	class Window
	{
		struct Vars;
		Vars *v;
		void init(int, int, int, int, bool);
		friend void cui::Menu::set_arg(Window &, const char *const [], const int *, const char *, int, int, int, int, int);
	public:
		Window(int height, int width, int starty = -1, int startx = -1, bool border = true);
			//  if @height or @width is equal to 0, the height or width of the screen will be assigned to it.
			//  if @startx or @starty is equal to -1, the menu will be put in the middle of the screen.
		Window(const Window &win, int heigt, int width, int starty = -1, int startx = -1, bool border = false);
			//  the window's position is based on @win.
			//  if @startx or @starty is equal to -1, the menu will be put in the middle of the screen.
		Window();
			// a full screen window
		int get_nlines() const;
		int get_ncols() const;
		void printf(const char *fmt, ...);
		int print_longmsg(const char *msg, int key_return = -1);
			// keys such as UP and DOWN can be used to scroll the screen. @key_return == -1 means any key.
			// return value is the key pressed.
		void printf_color(Cui_color color, const char *fmt, ...);
		void mvprintf(int y, int x, const char *fmt, ...);
		void clean();
		void free();
		void set_title(const char *str);
			// the window must has a border
		void set_scrollable(bool val);
		void draw_hline(int starty);
		~Window();
	};
}

#endif
