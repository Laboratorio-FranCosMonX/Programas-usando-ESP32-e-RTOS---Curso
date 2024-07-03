#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define LED_BUILTIN 2
#define STATE BIT0

EventGroupHandle_t xStatus_handle_eventGroup;

void init_hardware(){
    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
}

void tarefa_led(void *args){
    uint32_t state = 0;
    while(1){
        state = !state;
        gpio_set_level(LED_BUILTIN, state);
        if(state){
            xEventGroupSetBits(xStatus_handle_eventGroup, STATE);
        }else{
            xEventGroupClearBits(xStatus_handle_eventGroup, STATE);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL);
}

void tarefa_event_group(void * args){
    EventBits_t evento;
    while(1){
        evento = xEventGroupWaitBits(xStatus_handle_eventGroup, STATE, pdTRUE, pdFALSE, pdMS_TO_TICKS(2000));
        if(evento & STATE){
            printf("Estado: Ligado\n");
        }else{
            printf("Estado: Desligado\n");
        }
    }
    vTaskDelete(NULL);
}

void app_main(void){
    xStatus_handle_eventGroup = xEventGroupCreate();
    if(xStatus_handle_eventGroup == NULL){
        printf("Event group não foi criado.\n");
        return;
    }

    init_hardware();

    xTaskCreate(tarefa_led, "tarefa do LED", 1023, NULL, 2, NULL);
    xTaskCreate(tarefa_event_group, "tarefa do event group", 1023, NULL, 2, NULL);
}