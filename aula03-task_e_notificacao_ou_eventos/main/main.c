#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define LED_BUILTIN GPIO_NUM_2
#define BTN GPIO_NUM_5

SemaphoreHandle_t semph_button_handle;
volatile TickType_t valor_bouncing;

void init_hw(void){
    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_BUILTIN, 0);
    gpio_set_direction(BTN, GPIO_MODE_INPUT);
}

void task_liga_led(void *args){
    uint8_t status_led = 0;

    while(1){
        if(xSemaphoreTake(semph_button_handle,pdMS_TO_TICKS(500)) == pdTRUE){ //tempo de espera. o mesmo que fazer a atividade esperar por 0,5 segs
            if((xTaskGetTickCount() - valor_bouncing) < 100){ //tempo de espera para evitar o eveito bouncing
                status_led = 1;
                gpio_set_level(LED_BUILTIN, status_led);
            }
        }else{
            status_led = 0;
            gpio_set_level(LED_BUILTIN, status_led);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void task_button(void *args){
    while(1){
        if(gpio_get_level(BTN) == 0){
            printf("btn apertado\n");
            valor_bouncing = xTaskGetTickCount();   //obter qtd de ticks atuais e se preparar para evitar efeito bouncing
            xSemaphoreGive(semph_button_handle);    //informar que o evento aconteceumentando o semaforo e colocando a 0
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void){
    //aplicacao: um flash de camera usando um semaforo
    init_hw();

    semph_button_handle = xSemaphoreCreateBinary();
    if(semph_button_handle == NULL){
        printf("Erro: falha ao criar semaforo binario.\n");
        return;
    }

    //criando uma atividade
    xTaskCreate(task_button, "tarefa do botao", 2048, NULL, 2, NULL);
    xTaskCreate(task_liga_led, "task 2", 2048, NULL, 2, NULL);
}
