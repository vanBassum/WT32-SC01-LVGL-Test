#pragma once
#include "framing.h"
#include "../com/IConnection.h"

class FramedConnection
{
	Framing framing;
	IConnection *con = NULL;
	
public:
	Callback<void, FramedConnection*, Frame*> FrameCollected;
	
private:
	
	void FrameCollectedCallback(Framing* sender, Frame* frame)
	{
		if (FrameCollected.IsBound())
			FrameCollected.Invoke(this, frame);
		else
			free(frame);
	}
	
	void DataReceivedCallback(IConnection* con, uint8_t* data, size_t size)
	{
		framing.HandleData(data, size);
	}
	
public:
	
	FramedConnection()
	{
		framing.FrameCollected.Bind(this, &FramedConnection::FrameCollectedCallback);
	}
	
	void SetConnection(IConnection* connection)
	{
		if (con != NULL)
			con->OnDataReceived.Unbind();
		con = connection;
		con->OnDataReceived.Bind(this, &FramedConnection::DataReceivedCallback);
	}
	
	void SendFrame(Frame* frame)
	{
		if (con != NULL)
		{
			std::vector<uint8_t> raw = Framing::ToBytes(frame);
			con->Send((uint8_t*)&raw[0], raw.size());
			raw.clear();
		}
	}
};