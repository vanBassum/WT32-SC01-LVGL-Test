#pragma once
	
#include "stdint.h"
#include "../Misc/Callback.h"
#include "../protocol/SoftwareID.h"
#include "../Misc/IJSON.h"
#include "../com/IConnection.h"
#include "Framing.h"

namespace JBV
{
	
	
	
	
	class ListenerInfo : public IJsonConvertable
	{
	public:
		enum class ProtTypes
		{
			TCP,
		};
		
		ProtTypes ProtocolType;

		virtual bool PopulateFromJSON(IJSON *json) = 0;
		virtual IJSON* ToJSON() = 0;

	};
		
	
	
	
	class TCPListenerInfo : public ListenerInfo
	{
	public:
		
		std::string ip;
		uint16_t port;
		
		
		TCPListenerInfo()
		{
			ProtocolType = ProtTypes::TCP;
		}
		
		
		virtual bool PopulateFromJSON(IJSON *json)
		{
			return false;
		}
		
		virtual IJSON* ToJSON()
		{
			IJSON* result = new IJSON();
			result->AddChild("ProtocolType", (int)ProtocolType);
			result->AddChild("Ip", ip);
			result->AddChild("Port", port);
			return result;
		}
		
	};
		

	
	class DiscoveryInfo : public IJsonConvertable
	{
	public:
		
		SoftwareID SID;
		uint64_t Address;
		std::vector<ListenerInfo*> Listeners;
		
		virtual bool PopulateFromJSON(IJSON *json)
		{
			
			return false;
		}
	
		virtual IJSON* ToJSON()
		{
			IJSON* json = new IJSON();
			
			json->AddChild("SID", (uint32_t)SID);
			json->AddChild("Address", Address);

			
			IJSON* listenersJson = new IJSON();
			
			for (int i = 0; i < Listeners.size(); i++)
			{
				IJSON* j = Listeners[i]->ToJSON();
				listenersJson->AddChild(std::to_string(i), j);
				delete j;
			}
			
			json->AddChild("Listeners", listenersJson);
			delete listenersJson;
			return json;
		}
	};
	
	
	
	
}
