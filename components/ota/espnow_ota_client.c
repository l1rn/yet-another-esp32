#include "espnow_ota_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "esp_now.h"
#include "esp_ota_ops.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ESP_NOW_OTA_CLIENT";

static esp_ota_handle_t ota_handle = 0;
static const esp_partition_t *update_partition = NULL;
static int64_t last_chunk_time = 0;
static bool ota_started = false;

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
		u8 primary_ch = CONFIG_ESP_PEER_CHANNEL;
		wifi_second_chan_t second_ch;
		esp_wifi_get_channel(&primary_ch, &second_ch);

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
	if (len < sizeof(ota_packet_t) - OTA_CHUNK_SIZE) return;   

	ota_packet_t *pkt = (ota_packet_t *)data;

	if (pkt->type == OTA_CMD_DATA) {
		uint32_t calc_crc = esp_rom_crc32_le(0, pkt->payload, pkt->data_len);
		if (calc_crc != pkt->crc32) {
		    ESP_LOGE(TAG, "CRC mismatch chunk %lu, discarding", pkt->chunk_idx);
		    return;
		}

		if (!ota_started) {
			update_partition = esp_ota_get_next_update_partition(NULL);
			if (update_partition == NULL) {
				ESP_LOGE(TAG, "No OTA update partition found!");
				return;
			}
			if (esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle) != ESP_OK) {
				ESP_LOGE(TAG, "OTA begin failed");
				return;
			}
			ota_started = true;
			ESP_LOGI(TAG, "OTA started, writing to partition %s", update_partition->label);
		}

		esp_err_t err = esp_ota_write(ota_handle, pkt->payload, pkt->data_len);

		if (err != ESP_OK) {
			ESP_LOGE(TAG, "OTA write error: %s", esp_err_to_name(err));
			esp_ota_abort(ota_handle);
			ota_started = false;
			return;
		}

		send_ack(src_mac, pkt->chunk_idx);
		ESP_LOGI(TAG, "Chunk %lu/%lu (%d bytes, rssi:%d dBm)",
		pkt->chunk_idx, pkt->total_chunks, pkt->data_len, rssi);

		if (pkt->chunk_idx == pkt->total_chunks - 1) {
			if (esp_ota_end(ota_handle) == ESP_OK && esp_ota_set_boot_partition(update_partition) == ESP_OK) {
				ESP_LOGI(TAG, "OTA success! Rebooting...");
				vTaskDelay(pdMS_TO_TICKS(1000));
				esp_restart();
			} 
			else {
				ESP_LOGE(TAG, "OTA end failed");
			}
			ota_started = false;
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
