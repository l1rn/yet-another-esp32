#include "wifi.h"
#include "esp_wifi.h"

static const char *TAG = "Wi-Fi Manager";

// SOFT AP CONFIG
#define ESP_WIFI_AP_SSID CONFIG_ESP_WIFI_AP_SSID
#define ESP_WIFI_AP_PASSWORD CONFIG_ESP_WIFI_AP_PASSWORD
#define ESP_WIFI_AP_CHANNEL CONFIG_ESP_WIFI_AP_CHANNEL
#define ESP_MAX_STA_CONN_AP CONFIG_ESP_MAX_STA_CONN_AP

// STA CONFIG
#define ESP_STA_SSID CONFIG_ESP_STA_SSID
#define ESP_STA_PASSWORD CONFIG_ESP_STA_PASSWORD
#define ESP_STA_MAX_RETRY CONFIG_ESP_STA_MAX_RETRY

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

void wifi_init_sta(){
	esp_err_t ret = nvs_flash_init();
	if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_LOGI(TAG, "Wifi is configured\n");

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_ip;
	esp_event_handler_instance_t instance_got_ip;


	ESP_ERROR_CHECK(esp_wifi_set)
}
