#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BTN GPIO_NUM_4
#define LED GPIO_NUM_23
#define DEBOUNCE_COUNTER_MAX 3
xTaskHandle handle_led;

void init_hw(void){
    gpio_set_direction(BTN, GPIO_MODE_INPUT);// Entrada digital
    gpio_pullup_en(BTN); // Habilitar o pullup interno

    gpio_set_direction(LED, GPIO_MODE_OUTPUT);// Saída digital
}

void task_button(void *pvParameters){
    uint8_t debounce_counter = 0;

    while(1){
        if(gpio_get_level(BTN)==0) /* Se botão for pressionado*/ {
            debounce_counter ++;

            if(debounce_counter >= DEBOUNCE_COUNTER_MAX){ //quando for 1 seg
                debounce_counter = 0;
                xTaskNotifyGive(handle_led);//notificar a led
                printf("Botão pressionado e tarefa led notificada\n");
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    vTaskDelete(NULL);
}

void task_led(void *pvParameters){
    uint32_t valor_da_notificacao;
    while(1){
        //if(xTaskNotifyWait(0,0,&valor_da_notificacao, portMAX_DELAY)){
        if(ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(2000))){
            //printf("O LED foi notificado: %u\n", valor_da_notificacao);
            gpio_set_level(LED, 1);
            vTaskDelay(pdMS_TO_TICKS(2000));
            gpio_set_level(LED, 0);
           
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void app_main(void){
    init_hw();
    xTaskCreate (task_button, "button", 2048, NULL, 2, NULL);
    xTaskCreate (task_led, "LED", 1024, NULL, 2, &handle_led);
}