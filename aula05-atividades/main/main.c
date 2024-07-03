#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#define LED_BULTIN GPIO_NUM_2

TimerHandle_t xPiscaLed_handle_timer;
xTaskHandle xled_handle_task;

void init_hw(void){
    gpio_set_direction(LED_BULTIN, GPIO_MODE_OUTPUT);
}

void vTimer_Pisca_led(TimerHandle_t xtimer){
    xTaskNotifyGive(xled_handle_task);
}

void task_altera_estado_led(void *args){
    uint8_t estado_do_led = 0;
    while(1){
        if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000))){
            estado_do_led = !estado_do_led;
            if(estado_do_led == 0){
                printf("LED: LOW\n");
            }else{
                printf("LED: HIGH\n");
            }
        }
        gpio_set_level(LED_BULTIN, estado_do_led);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void){
    init_hw();

    xTaskCreate(task_altera_estado_led, "pisca led", 2048, NULL, 2, &xled_handle_task);

    xPiscaLed_handle_timer = xTimerCreate("pisca led", pdMS_TO_TICKS(1000), pdTRUE, (void *)1, vTimer_Pisca_led);
    xTimerStart(xPiscaLed_handle_timer, pdMS_TO_TICKS(10000));

}

