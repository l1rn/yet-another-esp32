#include "wifi_component.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_component.h"
#include "http_server.h"
#include "ota.h"
#include "ring_buffer.h"
#include "temperature_sensor.h"

void app_main(void)
{
	init_uart();
	circular_buf_init();
	start_softap_sta();
	vTaskDelay(pdMS_TO_TICKS(2000));
	httpd_handle_t s = start_server();
	register_ota_uri(s);
	xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
	xTaskCreate(uart_to_socket_task, "uart_to_tcp", 3072, NULL, 5, NULL);
	init_temperature_config();

	int count = 0;
	while(1){
		LOG_TO_WEB("[SENSOR] Real temperature: %d\n", get_temperature());
		vTaskDelay(pdMS_TO_TICKS(2500));
	}
}
