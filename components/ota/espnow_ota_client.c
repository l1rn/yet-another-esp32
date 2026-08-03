#include "espnow_ota_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "esp_now.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ESP_NOW_OTA_CLIENT";

static QueueHandle_t ack_queue = NULL;

typedef struct {
	u32 acked_chunk;
	bool status_ok;
} ack_response_t;

static void send_ack(const u8 *dest_mac, u32 chunk_idx){
	ota_packet_t ack_pkt = {
		.type = OTA_CMD_ACK,
		.chunk_idx = chunk_idx
	};

	if(!esp_now_is_peer_exist(dest_mac)){
		esp_now_peer_info_t peer = {};
		memcpy(peer.peer_addr, dest_mac, 6);
		peer.channel = CONFIG_ESP_PEER_CHANNEL;
		peer.encrypt = false;
		esp_now_add_peer(&peer);
	}

	esp_now_send(dest_mac, (u8 *)&ack_pkt, sizeof(u8) + sizeof(u32));
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
static void espnow_recv_cb(const esp_now_recv_info_t *recv_info, const u8 *data, int len){
	u8 *src_mac = (u8 *) recv_info->src_addr;
	int rssi = recv_info->rx_ctrl->rssi;
#else
static void espnow_recv_cb(const u8 *mac_addr, const u8 *data, int len){
	u8 *src_mac = (u8 *) mac_addr;
	int rssi = 0
#endif
	if(len >= sizeof(ota_packet_t) - OTA_CHUNK_SIZE){
		ota_packet_t *pkt = (ota_packet_t *)data;
		if(pkt->type == OTA_CMD_ACK){
			ESP_LOGI(TAG, "Chunk %lu/%lu received (Length: %d, Signal RSSI: %d dBm)",
                        pkt->chunk_idx, pkt->total_chunks, pkt->data_len, rssi);

			send_ack(src_mac, pkt->chunk_idx);
		}
	}
}

esp_err_t espnow_ota_client_init(const u8 *recv_mac){
	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb));
	
	esp_now_peer_info_t peer = {};
	memcpy(peer.peer_addr, recv_mac, 6);
	peer.channel = CONFIG_ESP_PEER_CHANNEL;
	peer.encrypt = false;

	if(!esp_now_is_peer_exist(recv_mac))
		ESP_ERROR_CHECK(esp_now_add_peer(&peer));

	esp_wifi_set_ps(WIFI_PS_NONE);
	ESP_LOGI(TAG, "ESP-NOW OTA Client initialized for target: %02x:%02x:%02x:%02x:%02x:%02x", 
			recv_mac[0], recv_mac[1], recv_mac[2],
			recv_mac[3], recv_mac[4], recv_mac[5]);
	return ESP_OK;
}
