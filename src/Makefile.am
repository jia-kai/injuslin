#This file is part of injuslin, informatcis judge system for linux.
#
#Copyright (C) <2008, 2009>  Jiakai <jia.kai66@gmail.com>
#
#Injuslin is free software: you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation, either version 3 of the License, or
#(at your option) any later version.
#
#Injuslin is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with injuslin.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS = injuslin

injuslin_SOURCES = auto_add_points.cpp bstream.cpp common.cpp conf.cpp configure_contest.cpp gtk_addition.cpp judge_core.cpp \
				   main.cpp start_judge.cpp structures.cpp cui.cpp view_results.cpp filecmp.cpp

EXTRA_DIST = auto_add_points.h bstream.h common.h conf.h configure_contest.h gtk_addition.h judge_core.h start_judge.h \
			 structures.h cui.h view_results.h filecmp.h

injuslin_LDADD = -lncurses -lmenu -lpanel -lz -lpthread $(GTK_LIBS) $(LTLIBINTL)
injuslin_CPPFLAGS = $(GTK_CFLAGS)

install-exec-hook:
	chown nobody:nobody $(bindir)/injuslin
	chmod 6755 $(bindir)/injuslin


DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
 

