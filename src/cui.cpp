/*
 * $File: cui.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Sep 16 23:10:01 2009
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

#include "cui.h"
#include "common.h"

#include <ncurses.h>
#include <menu.h>
#include <panel.h>

#include <stdarg.h>

#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
using namespace std;

//--------------------------------Function Declaration---------------------------------
static void destroy_win(WINDOW *win);
static bool move_current_item(int key, MENU *menu, bool multiple_cols);
//-------------------------------------------------------------------------------------

typedef vector<string> Str_array;
typedef set<void*> Pvoid_set;

static Pvoid_set win_exists;

void cui::start_cui()
{
	assert(win_exists.size() == 0);
	initscr();
	start_color();
	if (has_colors())
	{
		init_pair(CCOLOR_BLACK, COLOR_BLACK, COLOR_WHITE);
		init_pair(CCOLOR_RED, COLOR_RED, COLOR_BLACK);
		init_pair(CCOLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
		init_pair(CCOLOR_YELLOW, COLOR_GREEN, COLOR_BLACK);
		init_pair(CCOLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
		init_pair(CCOLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(CCOLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
		init_pair(CCOLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
	}
	clear();
	curs_set(0);
	raw();
	keypad(stdscr, TRUE);
	noecho();
}

void cui::end_cui()
{
	for (Pvoid_set::iterator i = win_exists.begin(); i != win_exists.end(); i ++)
		static_cast<Window*>(*i)->free();
	win_exists.clear();
	clear();
	endwin();
}

void cui::update()
{
	update_panels();
	doupdate();
}

int cui::get_str_nlines(const char *str, int cols)
{
	if (cols <= 0)
		cols = COLS;
	int ret = 0, len = strlen(str);
	int curlen = 0;
	for (int i = 0; i < len; i ++)
		if (str[i] == '\n')
		{
			ret += (curlen % cols ? curlen / cols + 1 : curlen / cols);
			curlen = 0;
		} else curlen ++;
	ret += (curlen % cols ? curlen / cols + 1 : curlen / cols);
	return ret;
}

int cui::message_box(const char *msg, const char *title)
{
	int win_width = COLS - 10, len = strlen(msg);
	if (win_width > len)
		win_width = len + 5;
	if (win_width < 15)
		win_width = 15;
	int nlines = get_str_nlines(msg, win_width);
	if (nlines + 5 > LINES)
		nlines = LINES - 5;
	Window win_msg(nlines + 2, win_width);
	if (title != NULL)
		win_msg.set_title(title);
	else win_msg.set_title("Message");
	return win_msg.print_longmsg(msg);
}

int cui::get_ncols(){return COLS;}
int cui::get_nlines(){return LINES;}
int cui::get_key(){return getch();}

struct cui::Window::Vars
{
	WINDOW *win, *win_border;
	PANEL *win_panel, *win_border_panel;
	bool freed;
};

void cui::Window::init(int height, int width, int starty, int startx, bool border)
{
	v = new Vars;
	if (border)
	{
		v->win_border = newwin(height, width, starty, startx);
		box(v->win_border, 0, 0);
		v->win_border_panel = new_panel(v->win_border);
		v->win = newwin(height - 2, width - 2, starty + 1, startx + 1);
		v->win_panel = new_panel(v->win);
	} else
	{
		v->win = newwin(height, width, starty, startx);
		v->win_panel = new_panel(v->win);
		v->win_border = NULL;
		v->win_border_panel = NULL;
	}
	v->freed = false;
	win_exists.insert(this);
}

cui::Window::Window(int h, int w, int sy, int sx, bool b)
{
	if (h <= 0 || h > LINES)
		h = LINES;
	if (w <= 0 || w > COLS)
		w = COLS;
	if (sy == -1)
		sy = (LINES - h) / 2;
	if (sx == -1)
		sx = (COLS - w) / 2;
	if (sy + h > LINES)
		sy = LINES - h;
	if (sx + w > COLS)
		sx = COLS - w;
	init(h, w, sy, sx, b);
}

cui::Window::Window(const Window &win, int height, int width, int starty, int startx, bool border)
{
	if (height <= 0 || height > win.get_nlines())
		height = win.get_nlines();
	if (width <= 0 || width > win.get_ncols())
		width = win.get_ncols();
	if (starty == -1)
		starty = (win.get_nlines() - height) / 2;
	if (startx == -1)
		startx = (win.get_ncols() - width) / 2;
	if (starty + height > win.get_nlines())
		starty = win.get_nlines() - height;
	if (startx + width > win.get_ncols())
		startx = win.get_ncols() - width;
	init(height, width, starty + getbegy(win.v->win), startx + getbegx(win.v->win), border);
}

cui::Window::Window()
{
	init(0, 0, 0, 0, false);
}

cui::Window::~Window()
{
	free();
	delete v;
}

int cui::Window::get_nlines() const
{
	return getmaxy(v->win);
}

int cui::Window::get_ncols() const
{
	return getmaxx(v->win);
}

void cui::Window::free()
{
	if (v->freed)
		return;
	v->freed = true;
	del_panel(v->win_panel);
	destroy_win(v->win);
	if (v->win_border != NULL)
	{
		del_panel(v->win_border_panel);
		destroy_win(v->win_border);
	}
	assert(win_exists.erase(this) == 1);
}

void cui::Window::printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vw_printw(v->win, fmt, args);
	va_end(args);
}

void cui::Window::printf_color(Cui_color color, const char *fmt, ...)
{
	if (has_colors())
		wattron(v->win, COLOR_PAIR((int)color));
	va_list args;
	va_start(args, fmt);
	vw_printw(v->win, fmt, args);
	va_end(args);
	if (has_colors())
		wattroff(v->win, COLOR_PAIR(1));
}

void cui::Window::mvprintf(int y, int x, const char *fmt, ...)
{
	char tmp[CSTRING_MAX_LEN];
	va_list args;
	va_start(args, fmt);
	vsnprintf(tmp, CSTRING_MAX_LEN, fmt, args);
	va_end(args);
	mvwprintw(v->win, y, x, tmp);
}

void cui::Window::clean()
{
	werase(v->win);
}

int cui::Window::print_longmsg(const char *msg, int key_return)
{
	Str_array lines(1);
	int ccount = 0;
	for (const char *ptr = msg; *ptr != '\0'; ptr ++)
	{
		if (*ptr == '\n')
		{
			ccount = 0;
			lines.push_back(string());
			continue;
		}
		lines.back().append(1, *ptr);
		ccount ++;
		if (ccount == getmaxx(v->win))
		{
			ccount = 0;
			lines.push_back(string());
		}
	}
	if (lines.back().empty())
		lines.erase(lines.end() - 1);
	werase(v->win);
	for (Str_array::size_type i = 0; i < lines.size() && (int)i < getmaxy(v->win); i ++)
		mvwprintw(v->win, (int)i, 0, "%s", lines[i].c_str());
	int curline = 0;
	while (true)
	{
		update();
		int ch;
		while (true)
		{
			ch = getch();
			if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_PPAGE || ch == KEY_NPAGE)
				break;
			if (ch == key_return || key_return == -1)
				return ch;
		}
		if ((int)lines.size() <= getmaxy(v->win))
			continue;
		int delta = 0;
		switch (ch)
		{
			case KEY_UP : 
				delta = -3; break;
			case KEY_DOWN :
				delta = 3; break;
			case KEY_PPAGE :
				delta = -getmaxy(v->win) + 1; break;
			case KEY_NPAGE :
				delta = getmaxy(v->win) - 1; break;
		}
		if (curline + delta < 0)
			delta = 0 - curline;
		if (curline + delta + getmaxy(v->win) >= (int)lines.size())
			delta = lines.size() - 1 - curline - getmaxy(v->win);
		if (delta == 0)
			continue;
		mvwprintw(v->win, 0, 0, "");
		winsdelln(v->win, -delta);
		if (delta > 0)
		{
			int w_line = getmaxy(v->win) - delta;
			int orig_line = curline + getmaxy(v->win);
			for (int i = 0; i < delta; i ++)
				mvwprintw(v->win, w_line + i, 0, "%s", lines[orig_line + i].c_str());
			curline += delta;
		} else
		{
			curline += delta;
			delta = -delta;
			for (int i = 0; i < delta; i ++)
				mvwprintw(v->win, i, 0, "%s", lines[curline + i].c_str());
		}
	}
}

void cui::Window::set_title(const char *str)
{
	if (v->win_border == NULL)
		return;
	box(v->win_border, 0, 0);
	if (str == NULL)
		return;
	int len = strlen(str) + 2;
	mvwprintw(v->win_border, 0, (getmaxx(v->win_border) - len) / 2, " %s ", str);
}

void cui::Window::set_scrollable(bool val)
{
	scrollok(v->win, (val ? TRUE : FALSE));
}

void cui::Window::draw_hline(int starty)
{
	mvwhline(v->win, starty, 0, ACS_HLINE, getmaxx(v->win));
}

//================================================================================

struct cui::Menu::Vars
{
	bool deleted, multiple_cols;
	int nitems;
	ITEM **items;
	MENU *menu;
	WINDOW *menu_win, *menu_win_sub;
	PANEL *menu_win_panel, *menu_win_sub_panel;
	const int *key_allowable;
};

cui::Menu::Menu()
{
	v = new Vars;
	v->deleted = true;
}

cui::Menu::~Menu()
{
	free();
	delete v;
}

void cui::Menu::set_arg(Window &win, const char * const items[], const int *key_allowable_, const char *msg, 
		int height, int width, int starty, int startx, int ncols)
{
	if (!v->deleted)
		free();
	v->deleted = false;
	int msg_nlines = 0;
	if (msg != NULL)
	{
		win.clean();
		win.printf(msg);
		msg_nlines = get_str_nlines(msg, win.get_ncols());
	}
	if (height == 0)
		height = win.get_nlines() - msg_nlines;
	if (width == 0)
		width = win.get_ncols();
	if (height < 3)
		height = 3;
	if (width < 3)
		width = 3;
	if (msg_nlines + height > win.get_nlines())
		height = win.get_nlines() - msg_nlines;
	if (width > win.get_ncols())
		width = win.get_ncols();
	if (startx == -1)
		startx = (win.get_ncols() - width) / 2;
	if (starty == -1)
		starty = (win.get_nlines() - height - msg_nlines) / 2 + msg_nlines;
	v->menu_win = newwin(height, width, starty + getbegy(win.v->win), startx + getbegx(win.v->win));
	box(v->menu_win, 0, 0);
	v->menu_win_panel = new_panel(v->menu_win);
	v->menu_win_sub = derwin(v->menu_win, height - 2, width - 2, 1, 1);
	v->menu_win_sub_panel = new_panel(v->menu_win_sub);
	v->key_allowable = key_allowable_;
	v->nitems = 0;
	while (items[v->nitems])
		v->nitems ++;
	v->items = new ITEM*[v->nitems + 1];
	for (int i = 0; i < v->nitems; i ++)
		v->items[i] = new_item(items[i], "");
	v->items[v->nitems] = NULL;
	v->menu = new_menu(v->items);
	set_menu_win(v->menu, v->menu_win);
	set_menu_sub(v->menu, v->menu_win_sub);
	set_menu_mark(v->menu, "*");
	set_menu_format(v->menu, height - 2, ncols);
	v->multiple_cols = (ncols > 1);
	assert(post_menu(v->menu) == E_OK);
}

int cui::Menu::wait_key()
{
	assert(!v->deleted);
	int ch;
	while (true)
	{
		update();
		ch = getch();
		bool done = false;
		for (const int *i = v->key_allowable; *i; i ++)
			if (ch == *i)
			{
				done = true;
				break;
			}
		if (done)
			break;
		move_current_item(ch, v->menu, v->multiple_cols);
	}
	return ch;
}

bool cui::Menu::wait_move_key(int &ch)
{
	assert(!v->deleted);
	bool ret;
	while (true)
	{
		update();
		ch = getch();
		bool done = false;
		for (const int *i = v->key_allowable; *i; i ++)
			if (ch == *i)
			{
				done = true;
				ret = false;
				break;
			}
		if (done)
			break;
		if (move_current_item(ch, v->menu, v->multiple_cols))
		{
			ret = true;
			break;
		}
	}
	return ret;
}

int cui::Menu::multiple_choice(bool *result, char *items[])
{
	assert(!v->deleted);
	unpost_menu(v->menu);
	menu_opts_off(v->menu, O_ONEVALUE);
	set_menu_mark(v->menu, "->");
	assert(post_menu(v->menu) == E_OK);
	int ch;
	bool done = false;
	while (!done)
	{
		update();
		ch = getch();
		if (move_current_item(ch, v->menu, v->multiple_cols))
			continue;
		if (ch == v->key_allowable[0])
		{
			items[get_choice()][0] = (item_value(current_item(v->menu)) == TRUE ? ' ' : '*');
			menu_driver(v->menu, REQ_TOGGLE_ITEM);
		}
		else if (ch == v->key_allowable[1])
			for (int i = 0; i < item_count(v->menu); i ++)
			{
				items[i][0] = '*';
				set_item_value(v->items[i], TRUE);
			}
		else
			for (const int *i = v->key_allowable + 2; *i; i ++)
				if (*i == ch)
				{
					done = true;
					break;
				}
	}
	for (int i = 0; i < item_count(v->menu); i ++)
		result[i] = (item_value(v->items[i]) == TRUE);
	unpost_menu(v->menu);
	set_menu_mark(v->menu, "*");
	menu_opts_on(v->menu, O_ONEVALUE);
	post_menu(v->menu);
	return ch;
}

int cui::Menu::get_choice()
{
	ITEM *cur = current_item(v->menu);
	for (int i = 0; i < item_count(v->menu); i ++)
		if (v->items[i] == cur)
			return i;
	return 0;
}

void cui::Menu::free()
{
	if (v->deleted)
		return;
	v->deleted = true;
	unpost_menu(v->menu);
	free_menu(v->menu);
	for (int i = 0; i < v->nitems; i ++)
		free_item(v->items[i]);
	delete []v->items;
	del_panel(v->menu_win_sub_panel);
	delwin(v->menu_win_sub);
	del_panel(v->menu_win_panel);
	destroy_win(v->menu_win);
}

bool move_current_item(int key, MENU *menu, bool mulc)
{
	switch (key)
	{
		case KEY_LEFT	:
			if (mulc)
			{
				menu_driver(menu, REQ_LEFT_ITEM);
				break;
			}
		case KEY_UP		:
			menu_driver(menu, REQ_UP_ITEM);
			break;
		case KEY_RIGHT	:
			if (mulc)
			{
				menu_driver(menu, REQ_RIGHT_ITEM);
				break;
			}
		case KEY_DOWN	:
			menu_driver(menu, REQ_DOWN_ITEM);
			break;
		case KEY_PPAGE	:
			menu_driver(menu, REQ_SCR_UPAGE);
			break;
		case KEY_NPAGE	:
			menu_driver(menu, REQ_SCR_DPAGE);
			break;
		case KEY_HOME	:
			menu_driver(menu, REQ_FIRST_ITEM);
			break;
		case KEY_END	:
			menu_driver(menu, REQ_LAST_ITEM);
			break;
		default			:
			return false;
	}
	return true;
}

void destroy_win(WINDOW *win)
{
	wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	werase(win);
	delwin(win);
}

