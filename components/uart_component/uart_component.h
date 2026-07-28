#ifndef UART_COMPONENT_H
#define UART_COMPONENT_H

void init_uart(void);
void uart_to_socket_task(void *pvParameters);
void tcp_server_task(void *pvParameters);

#endif // UART_COMPONENT_H
