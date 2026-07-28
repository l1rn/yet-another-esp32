#include "wifi_component.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_component.h"
#include "http_server.h"
#include "ota.h"

void app_main(void)
{
	init_uart();
	start_softap_sta();
	vTaskDelay(pdMS_TO_TICKS(2000));
	start_server();
	xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
	xTaskCreate(uart_to_socket_task, "uart_to_tcp", 3072, NULL, 5, NULL);
}
