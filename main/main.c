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

#include <math.h>

// --------------- COMIENZO CONFIGURACIÓN ---------------

// dead zone of all joysticks
#define DEAD_ZONE 30

// por debajo de esta velocidad se frenan las ruedas
# define HEADROOM_FRENAR_RUEDA 0.1


// MAC address of the allowed controller
static const char * controller_addr_string = "40:8E:2C:63:4F:34";

// PINES DE LA CONEXIÓN A LOS MOTORES, CON EL DRIVER 8833, 

// ESTOS SON DE SALIDA ASÍ QUE TIENES QUE SER MENORES A 34.

// motor delantero derecho
#define PIN_MDD1 22//22 no va
#define PIN_MDD2 27//

// motor delantero izquierdo
#define PIN_MDI1 33 //Funciona
#define PIN_MDI2 32 // Funciona

// motor trasero derecho
#define PIN_MTD1 4  //Funciona   
#define PIN_MTD2 16 //Funciona

// motor trasero izquierdo
#define PIN_MTI1 19 //Funciona
#define PIN_MTI2 18 //Funciona

// --------- FIN CONFIGURACIÓN ------------

// define motors
#define MOTOR_DELANTERO_DERECHO 0
#define MOTOR_DELANTERO_IZQUIERDO 1
#define MOTOR_TRASERO_DERECHO 2
#define MOTOR_TRASERO_IZQUIERDO 3

// BUTTONS
//#define BUTTON_L3 256
//#define BUTTON_R3 512

// forward
#define DIRECTION_FORWARD 0
#define DIRECTION_BACKWARDS 1

// Defined in my_platform.c
struct uni_platform* get_my_platform(void);

// para almacenar los valores de los ejes:

static volatile int32_t g_joystick_rX = 0;
static volatile int32_t g_joystick_rY = 0;
static volatile int32_t g_joystick_X = 0;
static volatile int32_t g_buttons = 0;


static void on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static const char *TAG = "ON_CONTROLLER_DATA";

    //static uint8_t leds = 0;
    //static uint8_t enabled = true;
    //static uni_controller_t prev = {0};
    uni_gamepad_t* gp;

    // Optimization to avoid processing the previous data so that the console
    // does not get spammed with a lot of logs, but remove it from your project.
    //if (memcmp(&prev, ctl, sizeof(*ctl)) == 0) {
    //    return;
    //}
    //prev = *ctl;
    // Print device Id before dumping gamepad.
    // This could be very CPU intensive and might crash the ESP32.
    // Remove these 2 lines in production code.
    //    logi("(%p), id=%d, \n", d, uni_hid_device_get_idx_for_instance(d));
    //    uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            gp = &ctl->gamepad;

            //ESP_LOGI(TAG,"rX: %ld, rY: %ld",gp->axis_rx, gp->axis_ry);
            //ESP_LOGI(TAG,"X: %ld, Y: %ld",gp->axis_x, gp->axis_y);
            //ESP_LOGI(TAG,"L3: %d", gp->buttons);
            g_buttons = gp->buttons;
            g_joystick_X = gp->axis_x;
            g_joystick_rY = gp->axis_ry;
            g_joystick_rX = gp->axis_rx;


            break;
        default:
            break;
    }
}


static const char *TAG = "MAIN";



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
    ledc_channel.gpio_num = PIN_MDD2;
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

    ledc_channel_t ch1 = 0;
    ledc_channel_t ch2 = 0;

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

void frenar_motor(uint32_t motor){
    ledc_channel_t ch1 = 0;
    ledc_channel_t ch2 = 0;

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

    // set pin2
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch2, 1024);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch2);

    // set pin1
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ch1, 1024);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ch1);
}

double max_double(double a, double b) {
    return (a > b) ? a : b;
}

// ejecutamos en el core1, así tenemos cada tarea en un core
void updateMotorsTask(void){
    double delantero_izquierda=0;
    double delantero_derecha=0;
    double trasero_izquierda=0;
    double trasero_derecha=0;


    const char *TAG = "MOTORS";
    ESP_LOGI(TAG, "Starting motors task");
    ESP_LOGI(TAG, "Task running on core %d", xPortGetCoreID());

    int32_t joystick_rX = 0;
    int32_t joystick_rY = 0;
    int32_t joystick_X = 0;
    int32_t buttons = 0;

    double x = 0;
    double y = 0;
    double turn = 0;

    for(;;){
        //TODO maybe check for no changes to prevent useless updates
        uint32_t multiplier = 1024;

        // update locals
        joystick_rX = g_joystick_rX;
        joystick_rY = g_joystick_rY;
        joystick_X = g_joystick_X;
        buttons = g_buttons;

        if (buttons & BUTTON_THUMB_R){
            //ESP_LOGI(TAG, "BUUTN THUM R");
            frenar_motor(MOTOR_DELANTERO_DERECHO);
            frenar_motor(MOTOR_DELANTERO_IZQUIERDO);
            frenar_motor(MOTOR_TRASERO_DERECHO);
            frenar_motor(MOTOR_TRASERO_IZQUIERDO);
            
        } else{

            // dead area: 20 units in all axes
            if (abs(joystick_rX) < DEAD_ZONE) {joystick_rX = 0;}
            if (abs(joystick_rY) < DEAD_ZONE) {joystick_rY = 0;}
            if (abs(joystick_X) < DEAD_ZONE) {joystick_X = 0;}

            // normalize variables
            // from -512_511 to -1_1
            x = (double)((double)joystick_rX + (double)512) / (double)1023;
            x = (x - 0.50)*2.0;
            y = -(double)((double)joystick_rY + (double)512) / (double)1023;
            y = (y + 0.50)*2.0;
            turn = (double)((double)joystick_X + (double)512) / (double)1023;
            turn = (turn - 0.50)*2.0;

            //ESP_LOGE(TAG, "x: %.2f | y: %.2f | turn: %.2f", x, y, turn);


            double potencia = 0;

            double seno = 0;
            double coseno = 0;
            double maximo = 0;


            if (x == 0.0 && y == 0.0){
                potencia = 0;
                seno = 0;
                coseno = 0;
                maximo = 1;
            } else {
                double theta = atan2(y,x);
                potencia = hypot(x,y);

                seno = sin(theta - M_PI_4);
                coseno = cos(theta - M_PI_4);

                maximo = max_double(fabs(seno), fabs(coseno));
            }
            // calculamos 

            delantero_izquierda = potencia * coseno/maximo + turn;
            delantero_derecha = potencia * seno/maximo - turn;
            trasero_izquierda = potencia * seno/maximo + turn;
            trasero_derecha = potencia * coseno/maximo - turn;

            if (potencia + fabs(turn) > 1){
                delantero_izquierda = delantero_izquierda / (potencia + fabs(turn));
                delantero_derecha = delantero_derecha / (potencia + fabs(turn));
                trasero_derecha = trasero_derecha / (potencia + fabs(turn));
                trasero_izquierda = trasero_izquierda / (potencia + fabs(turn));
            }

            // calculate multiplier
            if ((buttons & BUTTON_SHOULDER_L) && (buttons & BUTTON_SHOULDER_L)){
                multiplier = 768;
            } else if (buttons & BUTTON_SHOULDER_R){
                multiplier = 1024;

            } else if (buttons & BUTTON_SHOULDER_L){
                multiplier = 512;
            } else {
                multiplier = 768;
            }


            //ESP_LOGI(TAG, "X: %+ld | rX: %+ld | rY: %+ld", joystick_X, joystick_rX, joystick_rY);
            //ESP_LOGI(TAG, "DI: %.2f | DD: %.2f | TD: %.2f | TI: %.2f ", delantero_izquierda, delantero_derecha, trasero_derecha, trasero_izquierda);
            //vTaskDelay(pdMS_TO_TICKS(1000));
            

            // MOTOR_DELANTERO_DERECHO
            if (fabs(delantero_derecha) < HEADROOM_FRENAR_RUEDA){
                frenar_motor(MOTOR_DELANTERO_DERECHO);
            } else if (delantero_derecha > 0){
                mover_motor(MOTOR_DELANTERO_DERECHO, DIRECTION_FORWARD, (int)round(fabs(delantero_derecha) * multiplier));
            } else {
                mover_motor(MOTOR_DELANTERO_DERECHO, DIRECTION_BACKWARDS,(int)round(fabs(delantero_derecha) * multiplier));
            }

            // MOTOR_DELANTERO_IZQUIERDO
            if (fabs(delantero_izquierda) < HEADROOM_FRENAR_RUEDA){
                frenar_motor(MOTOR_DELANTERO_IZQUIERDO);
            } else if (delantero_izquierda > 0){
                mover_motor(MOTOR_DELANTERO_IZQUIERDO, DIRECTION_FORWARD, (int)round(fabs(delantero_izquierda) * multiplier));
            } else {
                mover_motor(MOTOR_DELANTERO_IZQUIERDO, DIRECTION_BACKWARDS,(int)round(fabs(delantero_izquierda) * multiplier));
            }

            // MOTOR_TRASERO_DERECHO
            if (fabs(trasero_derecha) < HEADROOM_FRENAR_RUEDA){
                frenar_motor(MOTOR_TRASERO_DERECHO);
            } else if (trasero_derecha > 0){
                mover_motor(MOTOR_TRASERO_DERECHO, DIRECTION_FORWARD, (int)round(fabs(trasero_derecha) * multiplier));
            } else {
                mover_motor(MOTOR_TRASERO_DERECHO, DIRECTION_BACKWARDS,(int)round(fabs(trasero_derecha) * multiplier));
            }

            // MOTOR_TRASERO_IZQUIERDO
            if (fabs(trasero_izquierda) < HEADROOM_FRENAR_RUEDA){
                frenar_motor(MOTOR_TRASERO_IZQUIERDO);
            } else if (trasero_izquierda > 0){
                mover_motor(MOTOR_TRASERO_IZQUIERDO, DIRECTION_FORWARD, (int)round(fabs(trasero_izquierda) * multiplier));
            } else {
                mover_motor(MOTOR_TRASERO_IZQUIERDO, DIRECTION_BACKWARDS,(int)round(fabs(trasero_izquierda) * multiplier));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void on_device_disconnected(uni_hid_device_t* d) {
    logi("custom: device disconnected: %p\n", d);
    esp_restart();
}
// main se ejecuta en el core 0, se encarga del bluetooth y comunicaciones
void app_main(void)
{
    ESP_LOGI(TAG, "Starting main app");

    ESP_LOGI(TAG, "Task running on core %d", xPortGetCoreID());
    // setup bluetooth
    bd_addr_t controller_addr;
    sscanf_bd_addr(controller_addr_string, controller_addr);

    // set up PWM settings
    ledc_init();

    xTaskCreatePinnedToCore(
        updateMotorsTask,            // Task function
        "my_task",          // Task name
        8096,               // Stack size in bytes
        NULL,               // Parameter to pass to the task function
        1,                  // Task priority
        NULL,               // Task handle (can be NULL if not needed)
        1                   // Core ID (1 for core 1)
    );

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
    platform->on_device_disconnected = on_device_disconnected;

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