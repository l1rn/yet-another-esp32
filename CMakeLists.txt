cmake_minimum_required(VERSION 3.5)
set(COMPONENTS main system_info http_server uart_component wifi_component ota temperature_sensor
	ring_buffer)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(another-esp-project)
