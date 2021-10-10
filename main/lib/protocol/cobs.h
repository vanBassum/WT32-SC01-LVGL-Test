#pragma once

#include <stddef.h>
#include <stdint.h>


//http://www.stuartcheshire.org/papers/cobsforton.pdf

#define FinishBlock(X) (*code_ptr = (X), \
                        code_ptr = dst++, \
                        code = 0x01 )

#define DIVUP(n, d) ( (((n)-1)/(d)+1))

class COBS
{
public:

	static size_t CalcEncodedSize(size_t e)
	{
		return DIVUP(e, 254) + e + 1;
	}

	static size_t CalcDecodedSize(size_t e)
	{
		return (e - 1) - DIVUP(e - 1, 255);
	}


	static void Encode(uint8_t* dst, uint8_t* src, size_t decodedSize)
	{
		uint8_t* end = src + decodedSize;
		uint8_t* code_ptr = dst++;
		uint8_t code = 0x01;
		while (src < end)
		{
			if (*src == 0) FinishBlock(code);
			else
			{
				*dst++ = *src;
				code++;
				if (code == 0xFF) FinishBlock(code);
			}
			src++;
		}
		FinishBlock(code);
		*code_ptr = 0x00;
	}

	static void Decode(uint8_t* dst, uint8_t* src, size_t encodedSize)
	{
		const uint8_t* end = src + encodedSize;
		while (src < end)
		{
			int i, code = *src++;
			for (i = 1; i < code; i++) *dst++ = *src++;
			if (code < 0xFF) *dst++ = 0;
		}
	}
};