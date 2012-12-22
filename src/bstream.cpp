/*
 * $File: bstream.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sun Sep 20 12:06:00 2009
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

#include <cstring>
#include <vector>
#include <zlib.h>
#include <cstdio>
using namespace std;

#include "bstream.h"
#include "common.h"

typedef unsigned char Byte;
typedef unsigned long int Byte_size_t;
//-------------------------------Function Declaration--------------------
static void do_encrypt(const Byte *source, unsigned int slen, string &dest);
static void initdict_encrypt(char dict[64]);
static bool do_decrypt(const string &ciphertext, Byte *dest, Byte_size_t &dest_len);
static void initdict_decrypt(Byte dict[256]);
static bool encrypt(const Byte *plaintext, Byte_size_t plaintext_len, std::string &ciphertext);
static Byte * decrypt(const std::string &ciphertext, Byte_size_t &plaintext_len);
    // return NULL if failed. return value is created by new[], so it should be freed with delete[].
//-----------------------------------------------------------------------

struct Ibstream::Detail
{
	const Byte *data;
	Byte_size_t ndata, pos;
	bool good;
	void read(Byte *dest, Byte_size_t count);
};

void Ibstream::Detail::read(Byte *dest, Byte_size_t count)
{
	if (!good)
		throw Error();
	if (pos + count > ndata)
	{
		good = false;
		throw Error();
	}
	memcpy(dest, &data[pos], count);
	pos += count;
}

Ibstream::Ibstream(const char *str) : p(new Detail)
{
	p->data = decrypt(str, p->ndata);
	p->good = p->data != NULL;
	p->pos = 0;
}

Ibstream::~Ibstream()
{
	delete []p->data;
	delete p;
}

#define IMPL_TYPE(_t_) \
Ibstream& Ibstream::operator >> (_t_ &x) \
{ \
	p->read((Byte*)&x, sizeof(_t_)); \
	return *this; \
}

IMPL_TYPE(bool)
IMPL_TYPE(int)
IMPL_TYPE(unsigned long)
IMPL_TYPE(double)

#undef IMPL_TYPE

Ibstream& Ibstream::operator >> (string &str)
{
	if (!p->good)
		throw Error();
	str.clear();
	while (true)
	{
		char tmp = p->data[p->pos ++];
		if (tmp == 0)
			break;
		str.append(1, tmp);
		if (p->pos == p->ndata)
		{
			p->good = false;
			throw Error();
		}
	}
	return *this;
}

//============================================================================

struct Obstream::Detail
{
	static const Byte_size_t UNIT_LEN = 4096;
	vector<Byte*> data;
	Byte_size_t data_pos;
	string str;
	bool changed;
	void write(const Byte *src, Byte_size_t count);
};

void Obstream::Detail::write(const Byte *src, Byte_size_t count)
{
	changed = true;
	if (data_pos + count > UNIT_LEN)
	{
		Byte_size_t wlen = UNIT_LEN - data_pos;
		memcpy(*data.rbegin() + data_pos, src, wlen);
		data.push_back(new Byte[UNIT_LEN]);
		data_pos = 0;
		write(src + wlen, count - wlen);
		return;
	}
	memcpy(*data.rbegin() + data_pos, src, count);
	data_pos += count;
}

Obstream::Obstream() : p(new Detail)
{
	p->data.push_back(new Byte[p->UNIT_LEN]);
	p->data_pos = 0;
	p->changed = true;
}

Obstream::~Obstream()
{
	for (vector<Byte*>::iterator i = p->data.begin(); i != p->data.end(); i ++)
		delete []*i;
	delete p;
}

const char *Obstream::str()
{
	if (!p->changed)
		return p->str.c_str();
	Byte_size_t tlen = (p->data.size() - 1) * p->UNIT_LEN + p->data_pos;
	Byte* tmp = new Byte[tlen];
	for (vector<Byte*>::size_type i = 0; i < p->data.size() - 1; i ++)
		memcpy(tmp + p->UNIT_LEN * i, p->data[i], p->UNIT_LEN);
	memcpy(tmp + p->UNIT_LEN * (p->data.size() - 1), *p->data.rbegin(), p->data_pos);
	if (!encrypt(tmp, tlen, p->str))
	{
		delete []tmp;
		return NULL;
	}
	delete []tmp;
	p->changed = false;
	return p->str.c_str();
}

#define IMPL_TYPE(_t_) \
Obstream& Obstream::operator << (const _t_ &x) \
{ \
	p->write((const Byte*)&x, sizeof(_t_)); \
	return *this; \
}

IMPL_TYPE(bool)
IMPL_TYPE(int)
IMPL_TYPE(unsigned long)
IMPL_TYPE(double)

#undef IMPL_TYPE


Obstream& Obstream::operator << (const string &str)
{
	p->write((const Byte *)str.c_str(), str.length() + 1);
	return *this;
}


//  Transform binary data into visible text.

//-------------Data format:----------------
// |---p1----| |----p2---| |---p3--|
// 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 x
// |---char 0----| |----char 1---| (to mark whether char 1 is added to make the length even.)
//
// p1, p2 and p3 are in the dictionary, which can be represented as an visible character.

//-------------------------------Function Declaration--------------------
static void do_encrypt(const Byte *source, unsigned int slen, string &dest);
static void initdict_encrypt(char dict[64]);
static bool do_decrypt(const string &ciphertext, Byte *dest, Byte_size_t &dest_len);
static void initdict_decrypt(Byte dict[256]);
//-----------------------------------------------------------------------

bool encrypt(const Byte *plaintext, Byte_size_t plaintext_len, string &ciphertext)
{
	Byte_size_t dest_len = plaintext_len + 12;
	dest_len += dest_len / 100;
	Byte *dest = new Byte[dest_len];
	if (compress2(dest, &dest_len, plaintext, plaintext_len, 9) != Z_OK)
	{
		delete []dest;
		return false;
	}
	do_encrypt(dest, dest_len, ciphertext);
	delete []dest;
	char tmp[CSTR_INT_MAX_LEN];
	sprintf(tmp, "%lu ", plaintext_len);
	ciphertext.insert(0, tmp);
	return true;
}

Byte * decrypt(const string &ciphertext, Byte_size_t &plaintext_len)
{
	Byte *source = new Byte[ciphertext.length() / 3 * 2];
	Byte_size_t source_len;
	string::size_type realpos = 1;
	while (ciphertext[realpos - 1] != ' ')
		realpos ++;
	if (!do_decrypt(ciphertext.substr(realpos), source, source_len) || sscanf(ciphertext.c_str(), "%lu", &plaintext_len) != 1)
	{
		delete []source;
		return NULL;
	}
	Byte *plaintext = new Byte[plaintext_len];
	if (uncompress(plaintext, &plaintext_len, source, source_len) != Z_OK)
	{
		delete []source;
		delete []plaintext;
		return NULL;
	}
	delete []source;
	return plaintext;
}

void do_encrypt(const Byte *source, unsigned int slen, string &dest)
{
	dest.clear();
	static bool dict_inited = false;
	static char dict[64];
	if (!dict_inited)
		initdict_encrypt(dict);
	int t = slen / 2;
	for (int i = 0; i < t; i ++)
	{
		const Byte *data = &source[i * 2];
		unsigned int p1 = (*data >> 2);
		unsigned int p2 = (*data & 0x03) << 4;
		data ++;
		p2 |= (*data >> 4);
		unsigned int p3 = (*data & 0x0F) << 1;
		dest.append(1, dict[p1]);
		dest.append(1, dict[p2]);
		dest.append(1, dict[p3]);
	}
	if (slen % 2)
	{
		const Byte *data = &source[slen - 1];
		unsigned int p1 = (*data >> 2);
		unsigned int p2 = (*data & 0x03) << 4;
		unsigned int p3 = 1;
		dest.append(1, dict[p1]);
		dest.append(1, dict[p2]);
		dest.append(1, dict[p3]);
	}
}

void initdict_encrypt(char dict[64])
{
	int len = 0;
	for (char i = 'a'; i <= 'z'; i ++)
		dict[len ++] = i;
	for (char i = 'A'; i <= 'Z'; i ++)
		dict[len ++] = i;
	for (char i = '0'; i <= '9'; i ++)
		dict[len ++] = i;
	dict[len ++] = ':';
	dict[len ++] = ';';
}

bool do_decrypt(const string &ciphertext, Byte *dest, Byte_size_t &dest_len)
{
	static bool dict_inited = false;
	static Byte dict[256];
	if (!dict_inited)
		initdict_decrypt(dict);
	dest_len = 0;
	if (ciphertext.length() % 3)
		return false;
	for (string::const_iterator iter = ciphertext.begin(); iter != ciphertext.end(); iter ++)
	{
		Byte p1 = dict[(int)*iter];
		iter ++;
		Byte p2 = dict[(int)*iter];
		iter ++;
		Byte p3 = dict[(int)*iter];
		dest[dest_len ++] = (p1 << 2) | (p2 >> 4);
		dest[dest_len ++] = ((p2 & 0x0F) << 4) | (p3 >> 1);
	}
	if (dict[(int)*ciphertext.rbegin()] & 1)
		dest_len --;
	return true;
}

void initdict_decrypt(Byte dict[256])
{
	memset(dict, 0, sizeof(dict));
	Byte value = 0;
	for (char i = 'a'; i <= 'z'; i ++)
		dict[(unsigned int)i] = value++;
	for (char i = 'A'; i <= 'Z'; i ++)
		dict[(unsigned int)i] = value++;
	for (char i = '0'; i <= '9'; i ++)
		dict[(unsigned int)i] = value++;
	dict[(unsigned int)':'] = value ++;
	dict[(unsigned int)';'] = value ++;
}

