#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uart_component.h"
#include "ota.h"
#include "http_server.h"
#include "ring_buffer.h"
#include "temperature_sensor.h"
#include "esp_http_server.h"
#include "wifi_component.h"

void create_main_loop(){
#if CONFIG_ESP_ENABLE_UART
	init_uart();
#endif // CONFIG_ESP_ENABLE_UART
#if CONFIG_ESP_ENABLE_WEB_LOGS
	circular_buf_init();
#endif // CONFIG_ESP_ENABLE_WEB_LOGS
	init_temperature_config();
	start_softap_sta();
	if(wifi_is_connected()){
		httpd_handle_t s = start_server();
		register_ota_uri(s);
	}

#if CONFIG_ESP_ENABLE_UART
	xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
	xTaskCreate(uart_to_socket_task, "uart_to_tcp", 3072, NULL, 5, NULL);
#endif

	while(1){
		LOG_TO_WEB("[SENSOR] Real temperature: %d\n", get_temperature());
		vTaskDelay(pdMS_TO_TICKS(2500));
	}
}

void destroy_main_loop(){
	wifi_cleanup();
}
