#include "wifi.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_netif"
#include "esp_netif_net_stack.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// SOFT AP CONFIG
#define ESP_WIFI_AP_SSID CONFIG_ESP_WIFI_AP_SSID
#define ESP_WIFI_AP_PASSWORD CONFIG_ESP_WIFI_AP_PASSWORD
#define ESP_WIFI_AP_CHANNEL CONFIG_ESP_WIFI_AP_CHANNEL
#define ESP_MAX_STA_CONN_AP CONFIG_ESP_MAX_STA_CONN_AP

// STA CONFIG
#define ESP_STA_SSID CONFIG_ESP_STA_SSID
#define ESP_STA_PASSWORD CONFIG_ESP_STA_PASSWORD
#define ESP_STA_MAX_RETRY CONFIG_ESP_STA_MAX_RETRY
#define ESP_STA_CHANNEL CONFIG_ESP_STA_CHANNEL

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFI ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP 
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA__WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define DHCPS_OFFER_DNS 0x02

static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "Wifi STA";

static int s_retry_num = 0;

static void wifi_event_handler(
		void *arg, 
		esp_event_base_t event_base, 
		int32_t event_id, 
		void *event_data
) {
	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED){
		wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
		ESP_LOGI(TAG_AP, "Station "MACSTR" joined, AID=%d", MAC2STR(event->mac), event->aid);
	} else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED){
		wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
		ESP_LOGI(TAG_AP, "Station "MACSTR" left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
	} else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
		esp_wifi_connect();
		ESP_LOGI(TAG_STA, "Station started!");
	} else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
		ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
		ESP_LOGI(TAG_STA, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	} else if(event_base == IP_EVENT && event_id == IP_EVENT_ASSIGNED_IP_TO_CLIENT){
		const ip_event_assigned_ip_to_client_t *e (const ip_event_assigned_ip_to_client_t *)event_data;
		ESP_LOGI(TAG_AP, "Assigned IP to client: " IPSTR ", MAC=" MACSTR ", hostname='%s'", 
		IP2STR(&e->ip), MAC2STR(e->mac), e->hostname);
	}
}

esp_netif_t *wifi_init_sta(void){
	esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();
	
	wifi_config_t wifi_sta_config = {
		.sta = {
			.ssid = ESP_STA_SSID,
			.password = ESP_STA_PASSWORD,
			.channel = ESP_STA_CHANNEL,
			.failure_retry_cnt = ESP_STA_MAX_RETRY,
			.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
		},
	};

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
	ESP_LOGI(TAG_STA, "wifi_init_sta connected!");
	return esp_netif_sta;
}

esp_netif_t *wifi_init_softap(void){
	esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

	wifi_config_t wifi_ap_config {
		.ap = {
			.ssid = ESP_WIFI_AP_SSID,
			.ssid_len=strlen(ESP_WIFI_AP_SSID),
			.password = ESP_WIFI_AP_PASSWORD,
			.channel = ESP_WIFI_AP_CHANNEL,
			.max_connection = ESP_MAX_STA_CONN_AP,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.pmf_cfg = {
				.required = false,
			},
		},
	};

	if(strlen(ESP_WIFI_AP_PASSWORD) == 0){
		wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

	ESP_LOGI(TAG_AP, "wifi_init_softap connected! | SSID: %s | PASS: %s | CHANNEL: %d", ESP_WIFI_AP_SSID, ESP_WIFI_AP_PASSWORD, ESP_WIFI_AP_CHANNEL);
	return esp_netif_ap;
}





