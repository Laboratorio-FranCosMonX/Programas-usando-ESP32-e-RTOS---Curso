#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_AZUL GPIO_NUM_4

void app_main(void){
    gpio_pad_select_gpio(LED_AZUL);
    gpio_set_direction(LED_AZUL, GPIO_MODE_OUTPUT);

    while(1){
        gpio_set_level(LED_AZUL,1);
        //vTaskDelay(dp);
    }

}