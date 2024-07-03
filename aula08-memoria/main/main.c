#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BTN 4
#define LED 5

#define TAG "AULA 08:"
TaskHandle_t xLed_handle_task;
TaskHandle_t xBtn_handle_task;
TaskHandle_t xPot_handle_task;

void init_hw(void){
    gpio_config_t g_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .pin_bit_mask = 1 << BTN
    };

    gpio_config (&g_config);
    gpio_set_direction (LED, GPIO_MODE_OUTPUT);

    adc1_config_width (ADC_WIDTH_12Bit);
    adc1_config_channel_atten (ADC_CHANNEL_0, ADC_ATTEN_11db);
}

void task_btn(void * args){
    while(1){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_potenciometro(void * args){
    int valor_pot;
    long int adc_avr;
    int i;
    while(1){
        adc_avr = 0;
        for(i = 0 ; i < 64 ; i++){
            adc_avr = adc_avr + adc1_get_raw(ADC1_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        valor_pot = adc_avr/64;
        ESP_LOGI(TAG, "adc = %d", valor_pot);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void task_led(void * args){
    while(1){
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

uint32_t high_water_mark_tasks [1] = {0};
void app_main(void){
    init_hw();

    //xTaskCreate(task_btn, "tarefa do botao", 1024, NULL, 1, xBtn_handle_task);
    //xTaskCreate(task_led, "tarefa do led", 1024, NULL, 1, xLed_handle_task);
    xTaskCreate(task_potenciometro, "tarefa do potenciometro", 2048, NULL, 1, &xPot_handle_task);

    
    //vTaskGetInfo(xPot_handle_task, _xgerenciarTask_handle_task, task_detalhes);
    while(1){
        high_water_mark_tasks[0] = uxTaskGetStackHighWaterMark(xPot_handle_task);
        ESP_LOGE(TAG, "Stack, task pot = %u", high_water_mark_tasks[0]);
        ESP_LOGI(TAG, "Número de tasks: %u", uxTaskGetNumberOfTasks());// 1 para cada nucleo, 2 para gerenc de memor intern, a task, e 2 ??
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}