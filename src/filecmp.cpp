/*
 * $File: filecmp.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Wed Jul 06 10:53:31 2011 +0800
 */
/*
This file is part of injuslin, informatcis judge system for linux.

Copyright (C) <2008, 2009>  Jiakai <jia.kai66@gmail.com>

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

#include "filecmp.h"

#include <errno.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>

static const int BUF_SIZE = 32768, CHAR_EOF = 256;
// return CHAR_EOF on end of file
class Error{};
struct Freader
{
	FILE *fobj;
	char *buf, *buf_end, *ptr;

	Freader(const char *fpath);
	~Freader();

	int read_char();
};

double filecmp_normal(double fullscore, const char *std_input_path,
		const char *std_output_path, const char *user_output_path, void *data)
{
	try
	{
		Freader fstd(std_output_path), fusr(user_output_path);
		for (; ;)
		{
			int a = fstd.read_char(), b = fusr.read_char();
			if (a != b)
			{
				if (a == ' ')
					a = fstd.read_char();

				if (b == ' ')
					b = fusr.read_char();

				if (a == CHAR_EOF && b == '\n')
					b = fusr.read_char();
				if (a == '\n' && b == CHAR_EOF)
					a = fstd.read_char();

				if (a != b || (a != '\n' && a != CHAR_EOF))
					return 0;
			}

			if (a == CHAR_EOF)
				return fullscore;
		}
	} catch (Error)
	{
	}
	return 0;
}

Freader::Freader(const char *fpath) :
	fobj(fopen(fpath, "r")), buf(new char[BUF_SIZE]), buf_end(buf), ptr(buf)
{
	if (fobj == NULL)
		throw Error();
}

Freader::~Freader()
{
	delete []buf;
	if (fobj)
		fclose(fobj);
}

int Freader::read_char()
{
	int ret;
	do
	{
		if (buf_end == ptr)
		{
			buf_end = buf + fread(buf, 1, BUF_SIZE, fobj);
			ptr = buf;

			if (buf_end == ptr)
			{
				if (ferror(fobj))
					throw Error();
				return CHAR_EOF;
			}
		}
		ret = *(ptr ++);
	} while (ret == '\r');
	return ret;
}

