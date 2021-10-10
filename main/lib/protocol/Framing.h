#pragma once
#include <stddef.h>
#include <stdint.h>
#include "frames.h"
#include "cobs.h"



class Framing
{
	std::vector<uint8_t> buffer;
	
public:
	Callback<void, Framing*, Frame*> FrameCollected;

private:
	void ParseBuffer()
	{
		if (FrameCollected.IsBound())
		{
			Frame* frame = (Frame*) malloc(buffer.size());
			if (frame != NULL)
			{
				COBS::Decode((uint8_t*)frame, (uint8_t*)&buffer[0], buffer.size());
				FrameCollected.Invoke(this, frame);
				//free(frame);
			}
		}
	}
	
public:
	
	void HandleByte(uint8_t data)
	{
		uint8_t d = data;
		buffer.push_back(d);
		if (d == 0)
		{
			ParseBuffer();
			buffer.clear();
		}
	}
	
	void HandleData(uint8_t* data, size_t size)
	{
		for (int i = 0; i < size; i++)
		{
			HandleByte(data[i]);
		}
	}
	
	static std::vector<uint8_t> ToBytes(Frame* frame)
	{
		size_t decodedSize = frame->GetTotalSize();
		size_t encodedSize = COBS::CalcEncodedSize(decodedSize);
		std::vector<uint8_t> data(encodedSize);
		COBS::Encode((uint8_t*)&data[0], (uint8_t*)frame, decodedSize);
		return data;
	}
};


