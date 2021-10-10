#pragma once

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <string>
#include "../IConnection.h"
#include "../../rtos/task.h"
#include "../../misc/event.h"

#define ISSET(events, ev)	(events & (uint32_t)ev)

class UDPSocket : public IConnection
{
		
	
		
private:
	FreeRTOS::NotifyableTask *task;
	int _sock = 0;
	

	enum class State : uint8_t		//Don't change the existing numbers, its used for logging. You can add extra states.
	{
		Initializing,
		Binding,
		Running,
		Cleanup,
	};

	enum class Events : uint32_t	//These are flags!
	{
		StateChanged = (1 << 0),
		DoBind		 = (1 << 1),
	};

	void Work(void *arg)
	{
		ESP_LOGI("UDPSocket", "Task started");
		
		State prevState = State::Initializing;
		State actState 	= State::Initializing;
		State nextState = State::Initializing;

		int len;
		uint8_t rx_buffer[128];
		struct sockaddr_in dest_addr;

		while (true)
		{
			uint32_t events			= 0;
			task->GetNotifications(&events, 1000);	//When there are no events to be handled, the default cycletime is set by this timeout.
			switch(actState)
			{
				
			case State::Initializing:
				
				if (ISSET(events, Events::DoBind))
					nextState = UDPSocket::State::Binding;
				
				break;
				
				
			case State::Binding:
				{
					_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
				
					if (_sock >= 0)
					{
						int bc = 1;
						if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc)) >= 0) 
						{
							
							dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
							dest_addr.sin_family = AF_INET;
							dest_addr.sin_port = htons(51100);
							
							int err = bind(_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
							if (err == 0) 
								nextState = State::Running;
							else
							{
								ESP_LOGE("UDPSocket", "Socket unable to bind: errno %d, %s", errno, strerror(errno));
								nextState = State::Cleanup;
							}
						}
						else
						{
							ESP_LOGE("UDPSocket", "Failed to set sock options: errno %d", errno);
							nextState = State::Cleanup;
						}
					}
					else
					{
						ESP_LOGE("UDPSocket", "Unable to create socket: errno %d, %s", errno, strerror(errno));
						nextState = State::Cleanup;
					}
				}
				break;
				
			case State::Running:
				{
					struct sockaddr_storage source_addr;
					socklen_t socklen = sizeof(source_addr);
					int len = recvfrom(_sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

					if (len < 0) 
					{
						ESP_LOGE("UDPSocket", "Receive failed %d, %s", errno, strerror(errno));
					}
					else 
					{
						
						//char addr_str[128];
						//if (source_addr.ss_family == PF_INET) 
						//{
						//	inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
						//}
						//else if (source_addr.ss_family == PF_INET6) 
						//{
						//	inet6_ntoa_r(((struct sockaddr_in6 *)&source_addr)->sin6_addr, addr_str, sizeof(addr_str) - 1);
						//}
				
						if(OnDataReceived.IsBound())
							OnDataReceived.Invoke( this, rx_buffer, len);
					
					}
					break;
				}
				
			case State::Cleanup:
				shutdown(_sock, 0);
				close(_sock);
				vTaskDelay(2000 / portTICK_PERIOD_MS);
				nextState = State::Initializing;
				break;


			default:
				ESP_LOGE("UDPSocket", "State not handled'");
				break;
			}


			// -------------------------------
			// ---- State switching logic ----
			// -------------------------------

			if(actState != nextState)
			{
				ESP_LOGI("UDPSocket", "Statechange %d -> %d'", (int)actState, (int)nextState);
				prevState = actState;
				actState = nextState;
				
				task->Notify((uint32_t) Events::StateChanged);	//Setting this will skip the 1sec delay and let the new state know its its first cycle.
			}
		}
	}


public:

	
	void Connect(std::string host_ip, int port)
	{

		//inet_ntoa_r(dest_addr.sin_addr, host_ip.c_str(), host_ip.length());
		task->Notify((uint32_t) Events::DoBind);
	}


	void Send(uint8_t* data, uint32_t length)
	{
		struct sockaddr_in dest_addr;
		dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(51100);
		
		int to_write = length;
		while (to_write > 0)
		{
			
			int written = sendto(_sock, data + (length - to_write), to_write, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
			//int written = send(_sock, data + (length - to_write), to_write, 0);
			if (written < 0)
			{
				switch (errno)
				{
				case 119:
					break;
				case 9:
					break;
				default:
					ESP_LOGE("UDPSocket", "Error occurred during sending: errno %d, %s", errno, strerror(errno));
					break;
				}
				return;
			}
			to_write -= written;
		}
		
	}


	UDPSocket()
	{
		task = new FreeRTOS::NotifyableTask("UDPSocket", 6, 1024 * 3, this, &UDPSocket::Work);
		task->Run(NULL);
	}


	~UDPSocket()
	{
		delete task;
		shutdown(_sock, 0);
		close(_sock);
	}


};

