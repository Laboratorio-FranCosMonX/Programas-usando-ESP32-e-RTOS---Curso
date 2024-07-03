#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "AULA 08:"
#define LED GPIO_NUM_2
#define BTN_A GPIO_NUM_4
#define BTN_B GPIO_NUM_5
#define BTNS (1 << BTN_A) | (1 << BTN_B)

QueueHandle_t xQueue_handle;
TaskHandle_t xLed_handle_task;

void init_hardware(void){
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);

    gpio_config_t btn_config = {
        .intr_type = GPIO_INTR_HIGH_LEVEL,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .pin_bit_mask = BTNS
    };
    gpio_config(&btn_config);
}

void task_led_pisca(void *args){
    while(1){
        gpio_set_level(LED,1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED,0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_btn_A_incrementa(void *args){
    uint8_t debounce = 0;
    uint32_t valor_const = 1;
    while(1){
        if(gpio_get_level(BTN_A) == 0){
            debounce++;
            if(debounce > 2){
                debounce = 0;
                xQueueSendFromISR(xQueue_handle, (void *) &valor_const, pdFALSE);
                //sinalizar troca de contexto
                portYIELD_FROM_ISR(pdFALSE);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void task_btn_B_decrementa(void *args){
    uint8_t debounce = 0;
    uint32_t valor_const = -1;
    while(1){
        if(gpio_get_level(BTN_B) == 0){
            debounce++;
            if(debounce > 2){
                debounce = 0;
                xQueueSendFromISR(xQueue_handle, (void *) &valor_const, pdFALSE);
                //sinalizar troca de contexto
                portYIELD_FROM_ISR(pdFALSE);
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void task_contador(void *args){
    uint32_t contador = 0;
    uint32_t valor_receive;
    while(1){
        if(xQueueReceiveFromISR(xQueue_handle,(void *) &valor_receive, NULL)){
            if(contador + valor_receive == 0){
                ESP_LOGE(TAG, "O contador nao pode assumir valor negativo.");
            }else{
                contador = contador + valor_receive;
                ESP_LOGI(TAG, "Contador = %u.", contador);
            }
        }
        if(contador > 4){
            vTaskSuspend(xLed_handle_task);
        }else{
            vTaskResume(xLed_handle_task);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void){
    init_hardware();

    xQueue_handle = xQueueCreate(3, sizeof(uint32_t));
    if(xQueue_handle == NULL){
        ESP_LOGE(TAG, "Erro ao criar uma Queue.");
        return;
    }

    xTaskCreate(task_led_pisca, "Tarefa do led", 1024, NULL, 0, &xLed_handle_task);
    xTaskCreate(task_btn_A_incrementa, "Tarefa que incrementa o contador", 2048, NULL, 1, NULL);
    xTaskCreate(task_btn_B_decrementa, "Tarefa que decrementa o contador", 2048, NULL, 1, NULL);
    xTaskCreate(task_contador, "Tarefa de gerencia do contador", 2048, NULL, 0, NULL);
}