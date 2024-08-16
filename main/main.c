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
#include <driver/ledc.h>

// Defined in my_platform.c
struct uni_platform* get_my_platform(void);

// define motors
#define MOTOR_DELANTERO_DERECHO 0
#define MOTOR_DELANTERO_IZQUIERDO 1
#define MOTOR_TRASERO_DERECHO 2
#define MOTOR_TRASERO_IZQUIERDO 3

// forward
#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARDS 1

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

// SIN PWM

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


void ledc_init(){

    // usamos mismo timer para todos los 8 PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 4000,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // preparar motor delantero izquierdo

    // MDI1
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_MDI1,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // MDI2
    ledc_channel.gpio_num = PIN_MDI2;
    ledc_channel.channel = LEDC_CHANNEL_1;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // MDD1
    ledc_channel.gpio_num = PIN_MDD1;
    ledc_channel.channel = LEDC_CHANNEL_2;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    // MDD2
    ledc_channel.gpio_num = PIN_MDD1;
    ledc_channel.channel = LEDC_CHANNEL_3;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));


    //MTD1
    ledc_channel.gpio_num = PIN_MTD1;
    ledc_channel.channel = LEDC_CHANNEL_4;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    //MTD2
    ledc_channel.gpio_num = PIN_MTD2;
    ledc_channel.channel = LEDC_CHANNEL_5;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // MTI1
    ledc_channel.gpio_num = PIN_MTI1;
    ledc_channel.channel = LEDC_CHANNEL_6;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    // MTI2
    ledc_channel.gpio_num = PIN_MTI2;
    ledc_channel.channel = LEDC_CHANNEL_7;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

}
/*
    @param motor: MOTOR_DELANTERO_DERECHO, MOTOR_DELANTERO_IZQUIERDO, MOTOR_TRASERO_DERECHO, MOTOR_TRASERO_IZQUIERDO
    @param direction: DIRECTION_FORWARD, DIRECTION_BACKWARDS
    @param pwm_value: 2¹⁰ = 0-1024 range 
*/

void mover_motor(uint32_t motor, uint32_t direction ,uint32_t pwm_value){

    ledc_channel_t ch1;
    ledc_channel_t ch2;

    switch (motor){
        case MOTOR_DELANTERO_DERECHO:
            ch1 = LEDC_CHANNEL_2;
            ch2 = LEDC_CHANNEL_3;
        break;
        case MOTOR_DELANTERO_IZQUIERDO:
            ch1 = LEDC_CHANNEL_0;
            ch2 = LEDC_CHANNEL_1;
        break;
        case MOTOR_TRASERO_DERECHO:
            ch1 = LEDC_CHANNEL_4;
            ch2 = LEDC_CHANNEL_5;
        break;
        case MOTOR_TRASERO_IZQUIERDO:
            ch1 = LEDC_CHANNEL_6;
            ch2 = LEDC_CHANNEL_7;
        break;
    }


    if (direction == DIRECTION_FORWARD){

        // set pin2
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch2, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch2);

        // set pin1
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch1, pwm_value);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch1);

    } else { // DIRECTION_BACKWARDS
        // set pin1
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch1, 0);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch1);

        // set pin2
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch2, pwm_value);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch2);
    }

}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting main app");
    // setup bluetooth
    bd_addr_t controller_addr;
    sscanf_bd_addr(controller_addr_string, controller_addr);

    // set up PWM settings
    ledc_init();

    ESP_LOGI(TAG, "Finished setup");


    /*for (;;){
        avanzar(PIN_MDD1, PIN_MDD2);
        // avanzar
            ESP_LOGI(TAG, "AVANZAR");
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 8192));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        gpio_set_level(PIN_MDD2, 0);
        vTaskDelay(pdMS_TO_TICKS(5000));

            ESP_LOGI(TAG, "PARAR");
        // parada brusca
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        gpio_set_level(PIN_MDD2, 0);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }


    return;*/

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