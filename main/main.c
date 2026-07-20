#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void) {
    printf("--- ESP32 запущен успешно! --- 🚀\n");
    int count = 0;
    while (1) {
        printf("Итерация: %d\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));  
    }
}
