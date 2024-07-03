#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

#define TAG "AULA 06"

#define BTN 5
#define BTN_UP 4
#define BTNS ((1 << BTN) | (1 << BTN_UP))
#define LED 2

#define message_buffer_size 50
MessageBufferHandle_t xmsg_hundle;

void init_analogic(void){
  adc1_config_width(ADC_WIDTH_12Bit);//resoluçção em bits no canala
  adc1_config_channel_atten(ADC_CHANNEL_0, ADC_ATTEN_11db);
}
void init_hw(void){
  gpio_config_t g_config = {
    .intr_type = GPIO_INTR_POSEDGE,//borda de subida
    .mode  = GPIO_MODE_INPUT,
    .pull_up_en = 1,
    .pull_down_en = 0,
    .pin_bit_mask = BTNS,
  };
  gpio_config (&g_config);

  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
}

void task_btn (void *pvParameters)
{
   bool btn_is_press = false;
   uint32_t counter_press = 0;

   while(1)
   {
      if(!gpio_get_level(BTN) && !btn_is_press)
      {
         vTaskDelay(pdMS_TO_TICKS(50));
         ESP_LOGI(TAG, "Pressionou o botão");
         counter_press ++;
         //handle, mensag, tamanho, tempo
         xMessageBufferSend(xmsg_hundle, (void *) &counter_press, sizeof(counter_press), 0);
         btn_is_press = true;
      }
      else if (gpio_get_level(BTN) && btn_is_press)
      {
         ESP_LOGI(TAG, "Botão solto");
         btn_is_press = false;
      }
      vTaskDelay(pdMS_TO_TICKS(100));
   }
}

void task_led (void *pvParameters)
{
   uint32_t counter_btn = 0;
   while(1)
   {
      if(xMessageBufferReceive(xmsg_hundle, (void *) &counter_btn, sizeof(xmsg_hundle), portMAX_DELAY)){
        ESP_LOGW(TAG, "counter %d\n", counter_btn);
      }

      vTaskDelay(pdMS_TO_TICKS(100));
   }
}

void task_adc (void *pvParameters)
{
  init_analogic();
  int analog_value;
  while(1){
    analog_value = adc1_get_raw(ADC1_CHANNEL_0);//devolve o valor em milivolts. 0.2V 200miliv
    ESP_LOGI(TAG, "valor = %d mV\n", analog_value);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void app_main(void){
   init_hw();

   xmsg_hundle = xMessageBufferCreate(message_buffer_size);
  xTaskCreate(task_btn, "task btn", 1024*3, NULL, 2, NULL);
  xTaskCreate(task_led, "task led", 1024*3, NULL, 2, NULL);
  xTaskCreate(task_adc, "task adc", 2048, NULL, 2, NULL);
}