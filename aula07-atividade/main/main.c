#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

#define LED GPIO_NUM_5

#define TAG "AULA 07"
#define MSG_BUFFER_SIZE 50
MessageBufferHandle_t xmsg_handle;

void init_hw(void){
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
}

void init_analogic(void){
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(ADC_CHANNEL_0, ADC_ATTEN_11db);
}

void task_potenciometro(void *args){
    init_analogic();
    int valor_aanalogico;
    uint32_t state;
    while(1){
        valor_aanalogico = adc1_get_raw(ADC1_CHANNEL_0);
        if(valor_aanalogico >= 3000){
            state = 1;
            xMessageBufferSend(xmsg_handle, (void *) &state, sizeof(state), 0);
        }else{
            state = 0;
            xMessageBufferSend(xmsg_handle, (void *) &state, sizeof(state), 0);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task_receptora(void *args){
    uint32_t state;
    while(1){
        if(xMessageBufferReceive(xmsg_handle,(void *) &state, sizeof(xmsg_handle), portMAX_DELAY)){
            if(state != 0){
                ESP_LOGE(TAG, "Limiar ultrapassado\n");
            }
            gpio_set_level(LED, state);
        }
    }
}

void app_main(void){
    init_hw();

    xmsg_handle = xMessageBufferCreate(MSG_BUFFER_SIZE);

    xTaskCreate(task_potenciometro, "tarefa do potenciometro", 2048, NULL, 1, NULL);
    xTaskCreate(task_receptora, "tarefa de msg e led", 2048, NULL, 1, NULL);
}