#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_http_server.h"

httpd_handle_t start_server(void);
void LOG_TO_WEB(const char *format, ...);

#endif // HTTP_SERVER_H
