#include <stdio.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <soc/gpio_num.h>
#include <driver/gpio.h>

// motor delantero derecho
#define PIN_MDD1 14
#define PIN_MDD2 27

// motor delantero izquierdo
#define PIN_MDI1 12
#define PIN_MDI2 13

// motor trasero derecho
#define PIN_MTD1 26
#define PIN_MTD2 25

// motor trasero izquierdo
#define PIN_MTI1 33
#define PIN_MTI2 32

// estas funciónes hay que pasarle el pin1 y pin2 del motor que queremos que avance hacia adelante.

void parada_lenta(gpio_num_t pin1, gpio_num_t pin2){
    gpio_set_level(pin1, 0);
    gpio_set_level(pin2, 0);

}

void retroceder(gpio_num_t pin1, gpio_num_t pin2){
    gpio_set_level(pin1, 0);
    gpio_set_level(pin2, 1);

}

void avanzar(gpio_num_t pin1, gpio_num_t pin2){
    gpio_set_level(pin1, 1);
    gpio_set_level(pin2, 0);

}

void parada_brusca(gpio_num_t pin1, gpio_num_t pin2){
    gpio_set_level(pin1, 1);
    gpio_set_level(pin2, 1);

}



static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting main app");
    // pin set up as output
        uint64_t pin_mask = (1ULL << PIN_MDD1) |
                        (1ULL << PIN_MDD2) |
                        (1ULL << PIN_MDI1) |
                        (1ULL << PIN_MDI2) |
                        (1ULL << PIN_MTD1) |
                        (1ULL << PIN_MTD2) |
                        (1ULL << PIN_MTI1) |
                        (1ULL << PIN_MTI2);

    // Configure all 8 pins as output
    gpio_config_t io_conf = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(PIN_MDD1, 0);
    gpio_set_level(PIN_MDD2, 0);

    gpio_set_level(PIN_MDI1, 0);
    gpio_set_level(PIN_MDI2, 0);

    gpio_set_level(PIN_MTD1, 0);
    gpio_set_level(PIN_MTD2, 0);

    gpio_set_level(PIN_MTI1, 0);
    gpio_set_level(PIN_MTI2, 0);


    ESP_LOGI(TAG, "Finished setup");


    int counter = 0;

    for (;;){
        ESP_LOGI(TAG, "Loop: %d", counter);

        gpio_num_t pin1 = PIN_MTD1;
        gpio_num_t pin2 = PIN_MTD2;
        
        ESP_LOGI(TAG, "AVANZANDO");
        avanzar(pin1, pin2);
        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "PARADA LENTA");
        parada_lenta(pin1, pin2);
        vTaskDelay(pdMS_TO_TICKS(5000));

        ESP_LOGI(TAG, "RETROCESO");
        retroceder(pin1, pin2);
        vTaskDelay(pdMS_TO_TICKS(5000));


        ESP_LOGI(TAG, "PARADA BRUSCA");
        parada_brusca(pin1, pin2);
        vTaskDelay(pdMS_TO_TICKS(5000));



        counter++;
    } 
}