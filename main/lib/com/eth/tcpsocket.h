/*
 * TCPConnection.h
 *
 *  Created on: 23 Oct 2020
 *      Author: Bas
 */

#ifndef MAIN_TCPIP_TCPCLIENT_H_
#define MAIN_TCPIP_TCPCLIENT_H_


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <string>
#include "../IConnection.h"
#include "../../rtos/task.h"
#include "../../misc/event.h"

#define ISSET(events, ev)	(events & (uint32_t)ev)

class TCPSocket : public IConnection
{
public:
	Event<TCPSocket*> OnDisconnect;
	Event<TCPSocket*> OnConnected;
		
private:
	FreeRTOS::NotifyableTask *task;
	

	int sock = 0;
	bool autoReconnect = false;
	int addr_family = 0;
	int ip_protocol = 0;
	struct sockaddr_in dest_addr;

	enum class State : uint8_t		//Don't change the existing numbers, its used for logging. You can add extra states.
	{
		Initializing,
		DoConnect,
		Connected,
		Cleanup,
		Idle,
		Disposed,
	};

	enum class Events : uint32_t	//These are flags!
	{
		StateChanged		= (1<<0),
		Connect				= (1<<1),
		Disconnect			= (1<<2),
		DoReceive			= (1<<3),
	};	

	void Work(void *arg)
	{
		State prevState = State::Initializing;
		State actState 	= State::Initializing;
		State nextState = State::Initializing;

		int len;
		uint8_t rx_buffer[128];

		while (true)
		{
			uint32_t events			= 0;

			task->GetNotifications(&events, 1000);	//When there are no events to be handled, the default cycletime is set by this timeout.

			switch(actState)
			{
			case State::Initializing:
				if (ISSET(events, Events::StateChanged))
				{
					//On entry.
				}

				//Continuously


				//Transitions
				if(sock == 0)
					nextState = State::Idle;	//TODO This is where we should be waiting for wifi or ETH.
				else
					nextState = State::Connected;
				break;

				
			case State::Idle:
				if (ISSET(events, Events::Connect) || autoReconnect) 
				{
					nextState = State::DoConnect;
				}
				break;
				
			case State::DoConnect:
				sock = socket(addr_family, SOCK_STREAM, ip_protocol);
				if (sock >= 0)
				{
					int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in6));
					if (err == 0)
					{
						//Success
						nextState = State::Connected;
					}
					else
					{
						ESP_LOGE("TCPConnection", "Socket unable to connect: errno %d, %s", errno, strerror(errno));
						shutdown(sock, 0);
						close(sock);
					}
				}
				else
				{
					ESP_LOGE("TCPConnection", "Unable to create socket: errno %d, %s", errno, strerror(errno));
					shutdown(sock, 0);
					close(sock);
				}
				break;
			
			case State::Connected:
				if (ISSET(events, Events::StateChanged))
				{
					OnConnected.Invoke(this);
				}
					
				len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
				if(len > 0)
				{
					//Data recieved.
					if(OnDataReceived.IsBound())
						OnDataReceived.Invoke(this, rx_buffer, len);
					task->Notify((uint32_t) Events::DoReceive);	//This makes sure we immediately start receiveing again.
				}
				else if(len == 0)
				{
					//Disconnected
					nextState = State::Cleanup;
				}
				else
				{
					//Error while receiving
					ESP_LOGE("TCPConnection.h", "Error occurred during receiving: errno %d, %s", errno, strerror(errno));
					nextState = State::Cleanup;
				}
				break;

			case State::Cleanup:
				
				if (ISSET(events, Events::StateChanged))
				{
					OnDisconnect.Invoke(this);
					shutdown(sock, 0);
					close(sock);
				}

				
				if (ISSET(events, Events::Connect))
				{
					
					nextState = State::DoConnect;
				}
				else if( autoReconnect) 
				{
					vTaskDelay(1000 / portTICK_PERIOD_MS);
					nextState = State::DoConnect;
				}
				else
				{
					
					nextState = State::Disposed;
				}
				break;
				
			case State::Disposed:
				if (ISSET(events, Events::StateChanged))
				{
					OnDispose.Invoke(this);
				}
				break;
				
			default:
				ESP_LOGE("TCPConnection", "State not handled'");
				break;
			}
			



			// -------------------------------
			// ---- State switching logic ----
			// -------------------------------

			if(actState != nextState)
			{
				//ESP_LOGI("TCPConnection", "Statechange %d -> %d'", (int)actState, (int)nextState);
				prevState = actState;
				actState = nextState;
				
				task->Notify((uint32_t) Events::StateChanged);	//Setting this will skip the 1sec delay and let the new state know its its first cycle.
			}
		}
	}


public:



	void Send(uint8_t* data, uint32_t length)
	{
		int to_write = length;
		while (to_write > 0)
		{
			int written = send(sock, data + (length - to_write), to_write, 0);
			if (written < 0)
			{
				switch(errno)
				{
				case 119:
					break;
				case 9:
					break;
				default:
					ESP_LOGE("TCPConnection.h", "Error occurred during sending: errno %d, %s", errno, strerror(errno));
					break;
				}
				return;
			}
			to_write -= written;
		}
	}

	TCPSocket()
	{
		ESP_LOGI("DEBUG", "TCPSocket created");
		task = new FreeRTOS::NotifyableTask("TCPConnection", 6, 1024 * 3, this, &TCPSocket::Work);
		task->Run(NULL);
	}

	TCPSocket(int socket)
	{
		ESP_LOGI("DEBUG", "TCPSocket created");
		sock = socket;
		task = new FreeRTOS::NotifyableTask("TCPConnection", 6, 1024 * 3, this, &TCPSocket::Work);
		task->Run(NULL);
	}

	~TCPSocket()
	{
		ESP_LOGI("DEBUG", "TCPSocket deleted");
		delete task;
		shutdown(sock, 0);
		close(sock);
	}

	void Connect(std::string host_ip, int port, bool autoReconnect)
	{
		dest_addr.sin_addr.s_addr = inet_addr(host_ip.c_str());
		dest_addr.sin_family = AF_INET;
		dest_addr.sin_port = htons(port);
		addr_family = AF_INET;
		ip_protocol = IPPROTO_IP;
		this->autoReconnect = autoReconnect;
		task->Notify((uint32_t) Events::Connect);
	}


};




#endif

