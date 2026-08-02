idf_component_register(
	SRCS "main.c"               
	REQUIRES
		wifi_component
		uart_component
		http_server
		esp_http_server
		ota
		temperature_sensor
		ring_buffer
	)

