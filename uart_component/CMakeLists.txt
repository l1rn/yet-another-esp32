idf_component_register(
	SRCS "uart_component.c"
	INCLUDE_DIRS "."
	REQUIRES
		esp_driver_uart		
		esp_wifi
)
