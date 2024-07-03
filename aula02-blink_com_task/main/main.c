#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
//https://www.usinainfo.com.br/blog/esp32-tutorial-com-primeiros-passos/
#define LED_AZUL GPIO_NUM_4
#define LED_BRANCO GPIO_NUM_5
#define LED_AMARELO GPIO_NUM_23

TaskHandle_t xTask_handle_2;

void init_vh(){
    gpio_pad_select_gpio(LED_AZUL);
    gpio_set_direction(LED_AZUL, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_BRANCO);
    gpio_set_direction(LED_BRANCO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(LED_AMARELO);
    gpio_set_direction(LED_AMARELO, GPIO_MODE_OUTPUT);
}

//led azul
void task_1(void *args){
    while(1){
        printf("LED AZUL: LIGADO!\n");
        gpio_set_level(LED_AZUL, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("LED AZUL: DESLIGADO!\n");
        gpio_set_level(LED_AZUL, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

//led branco >> suspenso após 5 segundos do led amarelo ligado
void task_2(void *args){
    while(1){
        printf("LED BRANCO: LIGADO!\n");
        gpio_set_level(LED_BRANCO, 1);
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("LED BRANCO: DESLIGADO!\n");
        gpio_set_level(LED_BRANCO, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}

//led amarelo >> controlador
void task_3(void *args){
    //liga LED amarelo e suspende a atividade do branco, mas o azul continua normal
    while(1){
        printf("LED AMARELO: LIGADO... Tarefa 2 executando\n");
        gpio_set_level(LED_AMARELO,1);
        vTaskDelay(pdMS_TO_TICKS(5000));
        printf("LED AMARELO: DESLIGADO... Tarefa 2 suspensa\n");
        vTaskSuspend(xTask_handle_2);
        gpio_set_level(LED_AMARELO,0);
        vTaskDelay(pdMS_TO_TICKS(5000));
        vTaskResume(xTask_handle_2);
    }
    vTaskDelete(NULL);
}

void app_main(void){
    //inicializando as GPIOs
    init_vh();

    //criando tarefas
    xTaskCreate(task_1, "primeira tarefa", 1024, NULL, 2, NULL);
    xTaskCreate(task_2, "segunda tarefa", 1024, NULL, 2, &xTask_handle_2);
    xTaskCreate(task_3, "terceira tarefa", 2048, NULL, 2, NULL);
}