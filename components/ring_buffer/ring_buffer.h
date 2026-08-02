#ifndef RING_BUFFER_H
#define RING_BUFFER_H 

#ifndef CONFIG_MAX_LOG_LINES
#define CONFIG_MAX_LOG_LINES 100
#endif

#ifndef CONFIG_MAX_LINE_LEN
#define CONFIG_MAX_LINE_LEN 128;
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef uint8_t u8;

typedef struct {
	char logs[CONFIG_MAX_LOG_LINES][CONFIG_MAX_LINE_LEN];
	size_t head;
	size_t count;
	size_t max;
	SemaphoreHandle_t mutex;
} circular_buf_t; 

typedef circular_buf_t* cbuf_handle_t;

cbuf_handle_t circular_buf_init(void);
cbuf_handle_t circular_get_handle(void);
void circular_buf_put(cbuf_handle_t me, const char *msg);
int circular_buf_get_at(cbuf_handle_t me, size_t index, char *out_buf, size_t out_len);

#endif // RING_BUFFER_H
