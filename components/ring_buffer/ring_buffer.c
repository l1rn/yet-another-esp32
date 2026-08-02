#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static circular_buf_t cbuf;
static bool is_init = false;

cbuf_handle_t circular_buf_init(void){
	cbuf.max = CONFIG_MAX_LOG_LINES;
	cbuf.head = 0;
	cbuf.count = 0;
	cbuf.mutex = xSemaphoreCreateMutex();
	if(cbuf.mutex != NULL){
		is_init = true;
	}
	return &cbuf;
}

cbuf_handle_t circular_get_handle(void){
	if(!is_init){
		return NULL;
	}
	return &cbuf;
}

void circular_buf_put(cbuf_handle_t me, const char *msg){
	if(!me || !msg) return;

	if(xSemaphoreTake(me->mutex, pdMS_TO_TICKS(10)) == pdTRUE){
		snprintf(me->logs[me->head], CONFIG_MAX_LINE_LEN, "%s", msg);

		me->head = (me->head + 1) % me->max;
		if(me->count < me->max){
			me->count++;
		}

		xSemaphoreGive(me->mutex);
	}
}

int circular_buf_get_at(cbuf_handle_t me, size_t index, char *out_buf, size_t out_len){
	if(!me || !out_buf || index >= me->count) return -1;
	int status = -1;

	if(xSemaphoreTake(me->mutex, pdMS_TO_TICKS(10)) == pdTRUE){
		size_t tail = (me->count < me->max) ? 0 : me->head;
		size_t actual_idx = (tail + index) % me->max;

		snprintf(out_buf, out_len, "%s", me->logs[actual_idx]);
		status = 0;
		xSemaphoreGive(me->mutex);
	}
	return status;
}
