#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <stdarg.h>
#include "esp_event.h"

#include "http_server.h"
#include "ring_buffer.h"
#include "system_info.h"

static const char *TAG = "HTTP_SERVER";
static TaskHandle_t sse_task_handle = NULL;
static cbuf_handle_t g_cbuf_handle = NULL;
static bool web_logs = true;

void LOG_TO_WEB(const char *format, ...){
#if CONFIG_ESP_ENABLE_WEB_LOGS
	if(!g_cbuf_handle) return;

	char temp_buf[CONFIG_MAX_LINE_LEN];
	va_list args;
	va_start(args, format);
	vsnprintf(temp_buf, sizeof(temp_buf), format, args);
	va_end(args);

	circular_buf_put(g_cbuf_handle, temp_buf);
	if(sse_task_handle != NULL){
		xTaskNotifyGive(sse_task_handle);
	}
#else
	web_logs = false;
#endif
}

static esp_err_t root_get_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");
	char uptime[64];
	int uptime_len = get_uptime(uptime, sizeof(uptime));
	char info[256];
	int info_len = get_system_info(info, sizeof(info));
	char wifi_info[256];
	int wifi_len = get_sta_info(wifi_info, sizeof(wifi_info));
	
	httpd_resp_send_chunk(req, uptime, uptime_len);
	httpd_resp_send_chunk(req, info, info_len);
	httpd_resp_send_chunk(req, wifi_info, wifi_len);
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

static esp_err_t sse_events_handler(httpd_req_t *req){
	if(web_logs == false){
		httpd_resp_set_type(req, "text/html");
		httpd_resp_send(req, "ESP_ENABLE_WEB_LOGS disabled, to start track logs use <b>idf.py menuconfig</b> to change 'Application Configuration' from no to yes", HTTPD_RESP_USE_STRLEN);
		return ESP_OK;
	}
	httpd_resp_set_type(req, "text/event-stream");		
	httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
	httpd_resp_set_hdr(req, "Connection", "keep-alive");

	sse_task_handle = xTaskGetCurrentTaskHandle();

	if(!g_cbuf_handle){
		g_cbuf_handle = circular_get_handle();
		if(!g_cbuf_handle) return ESP_FAIL;
	}

	char line_buf[CONFIG_MAX_LINE_LEN];
	char send_buf[CONFIG_MAX_LINE_LEN + 16];

	size_t count = g_cbuf_handle->count;
	for(size_t i = 0; i < count; i++){
		if(circular_buf_get_at(g_cbuf_handle, i, line_buf, sizeof(line_buf)) == 0){
			int len = snprintf(send_buf, sizeof(send_buf), "%s\n\n", line_buf);
			httpd_resp_send_chunk(req, send_buf, len);
		}
	}

	size_t last_head = g_cbuf_handle->head;
	while(1){
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
		while(last_head != g_cbuf_handle->head){
			if(circular_buf_get_at(g_cbuf_handle, last_head, line_buf, sizeof(line_buf)) == 0){
				int len = snprintf(send_buf, sizeof(send_buf), "%s\n\n", line_buf);
				if(httpd_resp_send_chunk(req, send_buf, len) != ESP_OK){
					sse_task_handle = NULL;
					return ESP_OK;
				}
			}
			last_head = (last_head + 1) % g_cbuf_handle->max;
		} 
	}

	sse_task_handle = NULL;
	return ESP_OK;
}

static const httpd_uri_t root = {
	.uri = "/",
	.method = HTTP_GET,
	.handler = root_get_handler
};

static const httpd_uri_t events = {
	.uri = "/events",
	.method = HTTP_GET,
	.handler = sse_events_handler
};

httpd_handle_t start_server(void){
	httpd_handle_t server = NULL;

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_open_sockets = 5;
	config.lru_purge_enable = true;
	g_cbuf_handle = circular_get_handle();

	if(httpd_start(&server, &config) == ESP_OK){
		httpd_register_uri_handler(server, &root);
		httpd_register_uri_handler(server, &events);

		LOG_TO_WEB("[%s] Server started on port: %d", TAG, config.server_port);
		return server;
	}

	ESP_LOGI(TAG, "Failed to start a server on port: %d (kill the process on this port)", config.server_port);

	return NULL;
}

