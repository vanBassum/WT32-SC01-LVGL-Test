#pragma once


#include "esp_log.h"
#include "esp_now.h"
#include "string.h"
#include "../misc/callback.h"
const uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };



class ESPNOW
{
public:
	Callback<void, const uint8_t*, size_t> OnBroadcast;
	
	static ESPNOW* intern;
	
private:
	
	
	
	static void rx(const uint8_t *mac_addr, const uint8_t *data, int data_len)
	{
		
		//if (memcmp(mac_addr, broadcastAddress, 6))
		//	ESPNOW::intern->OnBroadcast.Invoke(data, data_len);
		
		ESP_LOGI("ESP NOW", "RX %d = %s", data_len, (char*)data);
	}

	static void tx(const uint8_t *mac_addr, esp_now_send_status_t status)
	{
		ESP_LOGI("ESP NOW", "Status = %d", (int)status);
	}
	
	void AddBroadcastPeer()
	{
		esp_now_peer_info_t *peer = (esp_now_peer_info_t*)malloc(sizeof(esp_now_peer_info_t));
		if (peer == NULL) {
			ESP_LOGE("ESP NOW", "Malloc peer information fail");
			esp_now_deinit();
			return;
		}
	
		memset(peer, 0, sizeof(esp_now_peer_info_t));
		peer->channel = 0;
		peer->ifidx = WIFI_IF_STA;
		peer->encrypt = false;
		memcpy(peer->peer_addr, broadcastAddress, ESP_NOW_ETH_ALEN);
		ESP_ERROR_CHECK(esp_now_add_peer(peer));
		free(peer);
	}
	
public:
	
	
	void Init()
	{
		//intern = this;
		if (esp_now_init() != ESP_OK) 
		{
			ESP_LOGE("ESP NOW", "Failed to init");
			return;
		}
		
		esp_now_register_send_cb(&ESPNOW::tx);
		esp_now_register_recv_cb(&ESPNOW::rx);
		AddBroadcastPeer();
	}
	
	void SendBroadcast(const uint8_t* src, size_t len)
	{
		ESP_ERROR_CHECK(esp_now_send(broadcastAddress, src, len));
	}
	
	
};

