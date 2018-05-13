/*
Copyright (C) 2018  GridRF Radio Team(tech@gridrf.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef __BASE64_H__
#define __BASE64_H__

#include <stdint.h>

class Base64
{
public:
	Base64();
	~Base64();

	int bin_to_b64(const uint8_t * in, int size, char * out, int max_len);
	int b64_to_bin(const char * in, int size, uint8_t * out, int max_len);
private:
	char code_to_char(uint8_t x);
	uint8_t char_to_code(char x);
	int b64_to_bin_nopad(const char * in, int size, uint8_t * out, int max_len);
	int bin_to_b64_nopad(const uint8_t * in, int size, char * out, int max_len);
};

#endif

