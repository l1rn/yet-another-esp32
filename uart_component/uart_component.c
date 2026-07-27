#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_wifi.h"

#define DEV_UART_PORT     UART_NUM_2
#define DEV_TX_PIN        17
#define DEV_RX_PIN        16
#define DEV_BAUDRATE      115200

#ifdef CONFIG_USB_ACCESS
#define PC_UART_PORT      UART_NUM_0
#define PC_TX_PIN         1         
#define PC_RX_PIN         3
#define PC_BAUDRATE       115200
#endif

#define BUF_SIZE          1024

static const char *TAG = "uart_bridge";

int client_sock = -1;
typedef uint8_t u8;

void init_uart(void){
	uart_config_t dev_uart_config = {
		.baud_rate = DEV_BAUDRATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.source_clk = UART_SCLK_APB,
	};

	ESP_ERROR_CHECK(uart_param_config(DEV_UART_PORT, &dev_uart_config));
	ESP_ERROR_CHECK(uart_set_pin(DEV_UART_PORT, DEV_TX_PIN, DEV_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(DEV_UART_PORT, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0));
}

void uart_to_socket_task(void *pvParameters){
	u8 data[BUF_SIZE];
	while(1){
		int len = uart_read_bytes(DEV_UART_PORT, data, BUF_SIZE, pdMS_TO_TICKS(20));
		if(len > 0 && client_sock >= 0){
			send(client_sock, data, len, 0);
		}
	}
}

void tcp_server_task(void *pvParameters){
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(23),
		.sin_addr.s_addr = htonl(INADDR_ANY),
	};

	bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
	listen(listen_sock, 1);

	while(1){
		struct sockaddr_in source_addr;
		socklen_t addr_len = sizeof(source_addr);
		int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
		if(sock >= 0){
			ESP_LOGI(TAG, "Client connected");
			int keepalive = 1;
			int keepidle = 5;
			int keepinterval = 2;
			int keepcount = 3;

			setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
			setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
			setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepinterval, sizeof(keepinterval));
			setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcount, sizeof(keepcount));

			struct timeval timeout = { .tv_sec = 1, .tv_usec = 0 };
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

			client_sock = sock;
			u8 rx_buffer[BUF_SIZE];
			while(client_sock >= 0){
				int len = recv(client_sock, rx_buffer, sizeof(rx_buffer), 0);
				if(len > 0){
					uart_write_bytes(DEV_UART_PORT, (const char *)rx_buffer, len);
				} else if(len == 0){
					ESP_LOGI(TAG, "Client closed connection!");
					break;
				} else {
					if(errno != EAGAIN && errno != EWOULDBLOCK){
						ESP_LOGE(TAG, "Socket error (errno %d), closing", errno);
						break;
					}
				}
			}

			close(sock);
			client_sock = -1;
			ESP_LOGI(TAG, "Client disconnected");
		}
	}
}

