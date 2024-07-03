#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

SemaphoreHandle_t xled_handle_semaph;

void vTask_led_branco(void *args){
  while(1){
    if(xSemaphoreTake(xled_handle_semaph, portMAX_DELAY) > 0){
      printf("led white asceso\n");
      
      vTaskDelay(50);
      xSemaphoreGive(xled_handle_semaph);
    }
  }
}

void vTask_led_amarelo(void *args){
  while(1){
    if(xSemaphoreTake(xled_handle_semaph, portMAX_DELAY) > 0){
      printf("led yallow asceso\n");
      //xSemaphoreGive(xled_handle_semaph);
      vTaskDelay(50);
    }
  }
}

void app_main(void){
  xled_handle_semaph = xSemaphoreCreateCounting(2,2);//usando como se fosse recursos disponiveis, como cpus
  if(xled_handle_semaph == NULL){
    printf("falha ao criar o semaforo contador\n");
    return;
  }

  xTaskCreate(vTask_led_amarelo, "tarefa do led amarelo", 2048, NULL, 1, NULL);
  xTaskCreate(vTask_led_branco, "tarefa do led branco", 2048, NULL, 1, NULL);
}