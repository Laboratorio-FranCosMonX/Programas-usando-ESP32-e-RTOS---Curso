#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN 5
#define BTN_UP 4
#define LED_WHITE 2
#define BTNS (1<<BTN) | (1<<BTN_UP)

#define TAG "aula 06"

QueueHandle_t xbtn_handle_queue;
uint32_t limiteQueue;

void init_hw(){
    gpio_config_t g_config = {
        .intr_type = GPIO_INTR_DISABLE, 
        .mode = GPIO_MODE_INPUT,       
        .pull_up_en = 1,        
        .pull_down_en = 0,      
        .pin_bit_mask = BTNS  
    };
    gpio_config(&g_config);
    
    gpio_set_direction(LED_WHITE, GPIO_MODE_OUTPUT);
}

void task_btn(void *arg){
    uint32_t contador = 0;
    while(1){
        if(gpio_get_level(BTN_UP) == 0){
            ESP_LOGI(TAG, "pressionou o botao");
            contador++;
            xQueueSend(xbtn_handle_queue, (void *)&contador, 0);
            vTaskDelay(100);
        }
        vTaskDelay(10);
    }
}

void task_led(void *arg){
    uint32_t counter_btn = 0;
    uint32_t status_led = 0;
    while(1){
        if(xQueueReceive(xbtn_handle_queue, &counter_btn, portMAX_DELAY)){
            ESP_LOGI(TAG, "Deu certo: %u\n", counter_btn);
            status_led = !status_led;
            gpio_set_level(LED_WHITE, status_led);
        }
        vTaskDelay(10);
    }
}

void app_main(void){
    init_hw();

    xbtn_handle_queue = xQueueCreate(5, sizeof(uint32_t));
    if(xbtn_handle_queue == NULL){
        ESP_LOGI(TAG, "erro ao alocar a fila\n");
        return;
    }

    xTaskCreate(task_btn, "tarefa do btn", 2048, NULL, 2, NULL);
    xTaskCreate(task_led, "tarefa do led", 2048, NULL, 2, NULL);
}