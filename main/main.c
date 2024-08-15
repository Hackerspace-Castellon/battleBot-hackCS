#include <stdio.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <soc/gpio_num.h>
#include <driver/gpio.h>

#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>

#include <uni.h>

#include "sdkconfig.h"

// Defined in my_platform.c
struct uni_platform* get_my_platform(void);


// motor delantero derecho
#define PIN_MDD1 14
#define PIN_MDD2 27

// motor delantero izquierdo
#define PIN_MDI1 12
#define PIN_MDI2 13

// motor trasero derecho
#define PIN_MTD1 32
#define PIN_MTD2 33

// motor trasero izquierdo
#define PIN_MTI1 25
#define PIN_MTI2 26

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
    // hci_dump_open(NULL, HCI_DUMP_STDOUT);

    // Don't use BTstack buffered UART. It conflicts with the console.
#ifdef CONFIG_ESP_CONSOLE_UART
#ifndef CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
    btstack_stdio_init();
#endif  // CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE
#endif  // CONFIG_ESP_CONSOLE_UART

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();

    // hci_dump_init(hci_dump_embedded_stdout_get_instance());

    // Must be called before uni_init()
    uni_platform_set_custom(get_my_platform());

    // Init Bluepad32.
    uni_init(0 /* argc */, NULL /* argv */);

    // Does not return.
    btstack_run_loop_execute();

    return;

}