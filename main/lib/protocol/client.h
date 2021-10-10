#pragma once

#include "esp_system.h"
#include "framedconnection.h"
#include "discovery.h"
#include "SoftwareID.h"
#include <vector>
#include <algorithm>
#include <map>
#include "../rtos/queue.h"
#include "../rtos/task.h"
#include "../Misc/IJSON.h"
#include "../Misc/Version.h"

namespace JBV
{
	struct QueuedFrame
	{
		Frame* frame;
		FramedConnection* con;
	};

	struct Route
	{
		uint16_t quality = 0;
		FramedConnection* con = NULL;
		uint64_t addr;	//Address of client that is connected to the othersize of 'con'
	};
	
	class Client
	{
	public:
		Callback<std::vector<uint8_t>, Client*, std::vector<uint8_t>> HandleRequest;
		uint64_t myAddress;
		SoftwareID SID;
		
	private:
		FreeRTOS::Queue<QueuedFrame*> frameQueue = FreeRTOS::Queue<QueuedFrame*>(10);
		std::vector<FramedConnection*> framedConnections;
		FreeRTOS::Task *routingTask;
		std::map<uint64_t, Route*> routingTable;
		
		
		void HandleFrame(FramedConnection* connection, SmallRequestFrame* frame)
		{
			if (HandleRequest.IsBound())
			{
				std::vector<uint8_t> requestData(frame->Payload, frame->Payload + frame->PayloadSize);
				std::vector<uint8_t> responseData = HandleRequest.Invoke(this, requestData);
				SmallResponseFrame* response = SmallResponseFrame::Create(frame->Id, responseData);		
				if (response != NULL)
				{
					connection->SendFrame(response);
					free(response);
				}
			}
		}
		
		void HandleFrame(FramedConnection* connection, AppRequestFrame* frame)
		{
			if (HandleRequest.IsBound())
			{
				std::vector<uint8_t> requestData(frame->Payload, frame->Payload + frame->PayloadSize);
				std::vector<uint8_t> responseData = HandleRequest.Invoke(this, requestData);
				AppResponseFrame* response = AppResponseFrame::Create(myAddress, frame->SrcAddr, frame->Id, responseData);
				if (response != NULL)
				{
					connection->SendFrame(response);
					free(response);
				}
			}
		}
		
		
		
		void HandleFrame(FramedConnection* connection, ProtocolFrame* frame) 
		{
			ProtocolFrame::Print(frame);

			ProtocolFrame* response = NULL;
			switch (frame->CMD)
			{
			case ProtocolCommands::RequestID:
				response = ProtocolFrame::Create(myAddress, frame->SrcAddr, frame->Id, ProtocolCommands::ReplyID, "");
				break;
				
			default:
				ESP_LOGE("Client", "HandleFrame ProtocolFrame cmd %d not implemented", (int)frame->CMD);
				break;
				
			}
			if (response != NULL)
			{
				ProtocolFrame::Print(response);
				connection->SendFrame(response);
				free(response);
			}
		}
		
		
		
		void HandleFrame(FramedConnection* connection, AppResponseFrame* frame) 
		{
			ESP_LOGE("Client", "HandleFrame AppResponseFrame not implemented");
		}
		

		void RouteFrame(FramedConnection* connection, RoutingFrame* frame)
		{
			frame->Quality++;	//This could later be depending on the connection quality.

			//Check if we should handle this frame.
			if(frame->NxtAddr == BROADCASTADDR || frame->NxtAddr == UNKNOWNADDR || frame->NxtAddr == myAddress)
			{
				bool handleFrame = false;
				if (frame->DstAddr == BROADCASTADDR)
				{
					//First, update the route if nessesary
					Route* route = routingTable[frame->SrcAddr];

					//Create route if not yet exists
					if (route == NULL)
					{
						route = new Route();
						route->con = connection;
						route->quality = frame->Quality;
						route->addr = frame->PrvAddr;
						routingTable[frame->SrcAddr] = route;
						//TODO: retry sending frames that had unknown routes.
					}

					//Update route if a better route is found.
					if (frame->Quality < route->quality)
					{
						route->con = connection;
						route->addr = frame->PrvAddr;
						route->quality = frame->Quality;
					}

					//Only resend broadcast if it was received through the fasted route, otherwise let the broadcast starve.
					if (frame->Quality == route->quality && connection == route->con)
					{
						frame->PrvAddr = myAddress;
						for (int i = 0; i < framedConnections.size(); i++)
						{
							if (framedConnections[i] != connection)
							{
								framedConnections[i]->SendFrame(frame);
							}
						}

						handleFrame = true;
					}
				}
				else if (frame->DstAddr == UNKNOWNADDR && frame->DstAddr == myAddress)
				{
					//Addressed to us, so handle frame.
					handleFrame = true;
				}
				else
				{
					//Not addressed to us, so reroute.
					Route* route = routingTable[frame->SrcAddr];
					if (route == NULL)
					{
						//@TODO: Store this frame, send requestID via broadcast.
					}
					else
					{
						frame->NxtAddr = route->addr;
						frame->PrvAddr = myAddress;
						route->con->SendFrame(frame);
					}
				}

				ESP_LOGI("Client", "Handleframe = %d", handleFrame);

				if (handleFrame)
				{
					switch (frame->Type)
					{
					case FrameTypes::ApplicationRequest:
						HandleFrame(connection, (AppRequestFrame*)frame);
						break;
					case FrameTypes::ApplicationResponse:
						HandleFrame(connection, (AppResponseFrame*)frame);
						break;
					case FrameTypes::Protocol:
						HandleFrame(connection, (ProtocolFrame*)frame);
						break;
					default:
						ESP_LOGE("Client", "RouteFrame unknown type %d", (int)frame->Type);
						break;
					}
				}
			}
		}
		

		
		void FrameProcessingTask(void* args)
		{
			while (true)
			{
				QueuedFrame* qFrame = NULL;
				if (frameQueue.Dequeue(qFrame, 10000))
				{
					Frame* frame = qFrame->frame;
					FramedConnection* connection = qFrame->con;
					if (frame->CheckCRC())
					{
						switch (frame->Type)
						{
						case FrameTypes::SmallRequest:
							HandleFrame(connection, (SmallRequestFrame*)frame);
							break;
						case FrameTypes::ApplicationRequest:
						case FrameTypes::ApplicationResponse:
						case FrameTypes::Protocol:
							//All these frames contain routing information. So handle accordingly.
							RouteFrame(connection, (RoutingFrame*)frame);
							break;
							
						default:
							ESP_LOGE("Client", "RouteFrame unknown type %d", (int)frame->Type);
							break;
						}
					}
					else
					{
						ESP_LOGE("Client", "RouteFrame CRC invalid");
						Frame::Print(frame);
					}
					
					free(frame);
					delete qFrame;
				}
			}
		}
		
		void FrameReceived(FramedConnection* sender, Frame* frame)
		{
			QueuedFrame* qFrame = new QueuedFrame();
			qFrame->frame = frame;
			qFrame->con = sender;
			if (!frameQueue.Enqueue(qFrame, 1000))
			{
				free(frame);
				delete qFrame;
			}
		}
		
		void ConnectionDispose(IConnection* sender)
		{
			//TODO Find framed connection holding this connection and dispose all resources.
			ESP_LOGE("Client", "ConnectionDispose not implemented");
		}
		
	public:
		void AddConnection(IConnection* con)
		{
			FramedConnection* fCon = new FramedConnection();
			fCon->SetConnection(con);
			con->OnDispose.Bind(this, &Client::ConnectionDispose);
			fCon->FrameCollected.Bind(this, &Client::FrameReceived);
			framedConnections.push_back(fCon);
		}
		
		Client(uint64_t address, SoftwareID sid, Version version)
		{
			myAddress = address;

			ESP_LOGI("Client", "Client initialized ID = 0x%16llX SID = 0x%04X Version = %s", address, (int)sid, version.ToString().c_str());

			routingTask = new FreeRTOS::Task("Client", 7, 1024 * 8, this, &Client::FrameProcessingTask);
			routingTask->Run(NULL);
		}
	};
}