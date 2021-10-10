#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <string>
#include "../Misc/crc.h"

const uint64_t BROADCASTADDR = 0xFFFFFFFFFFFFFFFF;
const uint64_t UNKNOWNADDR = 0x0000000000000000;


enum class FrameTypes : uint8_t
{
	Unknown = 0,				//Used to catch and discard improper initialized frames.
	SmallRequest = 1,			//Smallest form of frame supported by this protocol.		6 bytes overhead.
	SmallResponse = 2,			//Smallest form of frame supported by this protocol.		6 bytes overhead.
	Protocol= 3,				//Used by the clients to maintain the network.
	ApplicationRequest = 4,		//Part of request response on application level.
	ApplicationResponse = 5,	//Part of request response on application level.
};

enum class ProtocolCommands : uint8_t
{
	Unknown          = 0,
	RequestID        = 1,
	ReplyID          = 2,
	DiscoveryRequest = 3,
	DiscoveryReply   = 4,
};

class Framing;



#pragma pack(1)
struct Frame
{
	FrameTypes Type;   
	uint8_t Id;   
	uint16_t PayloadSize;
	uint8_t Payload[0];

private:
	uint16_t CRCplaceholder;
public:

	uint16_t* GetCRC()
	{
		return (uint16_t*) &Payload[PayloadSize];
	}
	
	void CalcCRC()
	{
		*GetCRC() = CRC16_2((uint8_t*)this, PayloadSize + sizeof(Frame) - 2);
	}

	bool CheckCRC()
	{
		uint16_t crc = CRC16_2((uint8_t*)this, PayloadSize + sizeof(Frame) - 2);
		return crc == *GetCRC();
	}
	
	size_t GetTotalSize()
	{
		return PayloadSize + sizeof(Frame);
	}
	
	static void Print(Frame* f)
	{
		printf("Frame\n");
		printf("    Type	0x%02X\n", (uint8_t)f->Type);
		printf("    ID		0x%02X\n", f->Id);
		printf("    PSize	0x%04X\n", f->PayloadSize);
		printf("    Payload ");
		for (int i = 0; i < f->PayloadSize; i++)
			printf("%02x ", ((uint8_t*)f->Payload)[i]);
		printf("\n");
		printf("    CRC		0x%04X\n", *f->GetCRC());

		printf("    RAW		");

		for (int i = 0; i < f->PayloadSize + sizeof(Frame); i++)
			printf("%02x ", ((uint8_t*)f)[i]);
		printf("\n");

	}
};

#pragma pack(1)
struct SmallRequestFrame : Frame
{	
	uint8_t Data[0];
};

#pragma pack(1)
struct SmallResponseFrame : Frame
{
	uint8_t Data[0];

	static SmallResponseFrame* Create(uint8_t id, std::vector<uint8_t> data)
	{
		SmallResponseFrame* frame = (SmallResponseFrame*)malloc(sizeof(SmallResponseFrame) + data.size());
		if (frame != NULL)
		{
			frame->Type = FrameTypes::SmallResponse;
			frame->Id = id;
			frame->PayloadSize = sizeof(SmallResponseFrame) - sizeof(Frame) + data.size();
			memcpy(frame->Data, &data[0], data.size());
			frame->CalcCRC();
		}
		return frame;
	}
};

#pragma pack(1)
struct RoutingFrame : Frame
{
	uint16_t Quality;
	uint64_t SrcAddr;
	uint64_t PrvAddr;
	uint64_t NxtAddr;
	uint64_t DstAddr;
};

#pragma pack(1)
struct ProtocolFrame : RoutingFrame
{
	ProtocolCommands CMD;
	uint8_t Data[0];
	
	uint16_t GetDataSize()
	{
		return PayloadSize - sizeof(ProtocolFrame) + sizeof(Frame);
	}
	
	static void Print(ProtocolFrame* f)
	{
		printf("ProtocolFrame\n");
		printf("    Type	0x%02X\n",		(uint8_t)f->Type);
		printf("    ID		0x%02X\n",		f->Id);
		printf("    PSize	0x%04X\n",		f->PayloadSize);
		printf("    Quality	0x%04X\n",		f->Quality);
		printf("    SRC		0x%16llX\n",	f->SrcAddr);
		printf("    PRV		0x%16llX\n",	f->PrvAddr);
		printf("    NXT		0x%16llX\n",	f->NxtAddr);
		printf("    DST		0x%16llX\n",	f->DstAddr);
		printf("    CMD		0x%02X\n",		(uint8_t)f->CMD);
		printf("    CRC		0x%04X\n", *f->GetCRC());
		printf("    Payload ");
		for (int i = 0; i < f->PayloadSize; i++)
			printf("%02x ", ((uint8_t*)f->Payload)[i]);
		printf("\n");



		printf("    RAW		");

		for (int i = 0; i < f->PayloadSize + sizeof(Frame); i++)
			printf("%02x ", ((uint8_t*)f)[i]);
		printf("\n");
	}
	
	static ProtocolFrame* Create(uint64_t src, uint64_t dst, uint8_t id, ProtocolCommands cmd, std::string msg)
	{
		size_t msgSize = msg.size();
		ProtocolFrame* frame = (ProtocolFrame*)malloc(sizeof(ProtocolFrame) + msg.size());
		if (frame != NULL)
		{
			frame->Type = FrameTypes::Protocol;
			frame->Id = id;
			frame->PayloadSize = sizeof(ProtocolFrame) - sizeof(Frame) + msg.size();
			frame->Quality = 0;
			frame->SrcAddr = src;
			frame->DstAddr = dst;
			frame->CMD = cmd;
			memcpy(frame->Data, msg.c_str(), msgSize);
			frame->CalcCRC();
		}
		return frame;
	}
};

#pragma pack(1)
struct AppRequestFrame : RoutingFrame
{
	uint8_t Data[0];
	
	uint16_t GetDataSize()
	{
		return PayloadSize - sizeof(AppRequestFrame);
	}

};
#pragma pack(1)
struct AppResponseFrame : RoutingFrame
{
	uint8_t Data[0];
	
	uint16_t GetDataSize()
	{
		return PayloadSize - sizeof(AppResponseFrame);
	}
	
	static AppResponseFrame* Create(uint64_t src, uint64_t dst, uint8_t id, std::vector<uint8_t> data)
	{
		AppResponseFrame* frame = (AppResponseFrame*)malloc(sizeof(AppResponseFrame) + data.size());
		if (frame != NULL)
		{
			frame->Type = FrameTypes::ApplicationResponse;
			frame->Id = id;
			frame->PayloadSize = sizeof(AppResponseFrame) - sizeof(Frame) + data.size();
			frame->Quality = 0;
			frame->SrcAddr = src;
			frame->DstAddr = dst;
			memcpy(frame->Data, &data[0], data.size());
			frame->CalcCRC();
		}
		return frame;
	}
};



