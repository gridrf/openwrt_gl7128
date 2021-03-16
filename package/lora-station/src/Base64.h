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

