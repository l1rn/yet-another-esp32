idf_component_register(
	SRCS "ota.c" "espnow_ota_client.c"
	INCLUDE_DIRS "."
	REQUIRES app_update esp_http_server esp_timer esp_wifi
)
