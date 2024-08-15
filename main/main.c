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


static void on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static const char *TAG = "ON_CONTROLLER_DATA";

    static uint8_t leds = 0;
    static uint8_t enabled = true;
    static uni_controller_t prev = {0};
    uni_gamepad_t* gp;

    // Optimization to avoid processing the previous data so that the console
    // does not get spammed with a lot of logs, but remove it from your project.
    if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
        return;
    }
    prev = *ctl;
    // Print device Id before dumping gamepad.
    // This could be very CPU intensive and might crash the ESP32.
    // Remove these 2 lines in production code.
    //    logi("(%p), id=%d, \n", d, uni_hid_device_get_idx_for_instance(d));
    //    uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            gp = &ctl->gamepad;

            // Debugging
            // Axis ry: control rumble
            if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
                d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */,
                                                  255 /* weak magnitude */, 0 /* strong magnitude */);
            }
            // Buttons: Control LEDs On/Off
            if ((gp->buttons & BUTTON_B) && d->report_parser.set_player_leds != NULL) {
                d->report_parser.set_player_leds(d, leds++ & 0x0f);
            }
            // Axis: control RGB color
            if ((gp->buttons & BUTTON_X) && d->report_parser.set_lightbar_color != NULL) {
                uint8_t r = (gp->axis_x * 256) / 512;
                uint8_t g = (gp->axis_y * 256) / 512;
                uint8_t b = (gp->axis_rx * 256) / 512;
                d->report_parser.set_lightbar_color(d, r, g, b);
            }
            ESP_LOGI(TAG,"rX: %ld, rY: %ld",gp->axis_rx, gp->axis_ry);
            ESP_LOGI(TAG,"X: %ld, Y: %ld",gp->axis_x, gp->axis_y);


            break;
        default:
            break;
    }
}


static const char *TAG = "MAIN";

static const char * controller_addr_string = "40:8E:2C:63:4F:34";


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

    // setup bluetooth
    bd_addr_t controller_addr;
    sscanf_bd_addr(controller_addr_string, controller_addr);


    ESP_LOGI(TAG, "Finished setup");


    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();

    struct uni_platform* platform = get_my_platform();
    platform->on_controller_data = on_controller_data;

    uni_platform_set_custom(platform);

    // Init Bluepad32.
    uni_init(0 /* argc */, NULL /* argv */);


    uni_bt_allowlist_remove_all();
    uni_bt_allowlist_add_addr(controller_addr);
    uni_bt_allowlist_set_enabled(true);
    //uni_bt_enable_new_connections_safe(false);


    // Does not return.
    btstack_run_loop_execute();

    return;

}