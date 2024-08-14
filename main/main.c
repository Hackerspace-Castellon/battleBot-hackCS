#include <stdio.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting main app");
    int counter = 0;

    for (;;){
        ESP_LOGI(TAG, "waiting: %d", counter);
        vTaskDelay(pdMS_TO_TICKS(1000));
        counter++;
    } 
}