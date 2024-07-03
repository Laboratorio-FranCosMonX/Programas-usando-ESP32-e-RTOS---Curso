#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "dht.h"

#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "esp_log.h"

#define SSID "PetrDark"
#define PASSWORD "123456789"
#define ESP_MAXIMUM_RETRY 5

#define TAG "Aula 12 - Projeto final"
#define RETRY_NUM 5
int wifi_connect_status = 0;
int retry_num = 0;

#define PIN_POT ADC1_CHANNEL_0
#define LED_BULTIN 2
#define DHT_PIN 23
#define ESTADO_WIFI BIT0

EventGroupHandle_t xStatusRede_handle_eventGroup;
QueueHandle_t queue_handle_temperatura_e_calc;
TaskHandle_t task_handle_contador;
TaskHandle_t task_handle_temperatura;

void init_hardware(){

    // Configuração do pino do LED como saída
    gpio_pad_select_gpio(LED_BULTIN);
    gpio_set_direction(LED_BULTIN, GPIO_MODE_OUTPUT);

    //inicializacao do potenciometro
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(PIN_POT, ADC_ATTEN_11db);
}

char html_page[] = "<!DOCTYPE HTML><html>\n"
                    "<head>\n"
                    "  <title>Web Server</title>\n"
                    "  <meta http-equiv=\"refresh\" content=\"10\">\n"
                    "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                    "  <style>\n"
                    "    body {\n"
                    "      font-family: Arial, Helvetica, sans-serif;\n"
                    "      margin: 0;\n"
                    "      padding: 0;\n"
                    "      background-color: #f2f2f2;\n"
                    "    }\n"
                    "    .topnav {\n"
                    "      background-color: #333;\n"
                    "      overflow: hidden;\n"
                    "    }\n"
                    "    .topnav h3 {\n"
                    "      color: white;\n"
                    "      text-align: center;\n"
                    "      padding: 14px 0;\n"
                    "      margin: 0;\n"
                    "    }\n"
                    "    .content {\n"
                    "      text-align: center;\n"
                    "      padding: 50px;\n"
                    "    }\n"
                    "    .card {\n"
                    "      background-color: white;\n"
                    "      border-radius: 10px;\n"
                    "      padding: 20px;\n"
                    "      margin: 10px;\n"
                    "      display: inline-block;\n"
                    "      box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2);\n"
                    "      transition: 0.3s;\n"
                    "      width: 30%;\n"
                    "    }\n"
                    "    .card h4 {\n"
                    "      margin: 0;\n"
                    "    }\n"
                    "    .card span {\n"
                    "      font-size: 24px;\n"
                    "    }\n"
                    "  </style>\n"
                    "</head>\n"
                    "<body>\n"
                    "  <div class=\"topnav\">\n"
                    "    <h3>Cursor de Extensao RTOS</h3>\n"
                    "  </div>\n"
                    "  <div class=\"content\">\n"
                    "    <div class=\"cards\">\n"
                    "      <div class=\"card temperatura\">\n"
                    "        <h4>Temperatura</h4><p><span>%.2f&deg;C</span></p>\n"
                    "      </div>\n"
                    "      <div class=\"card fluxo_agua\">\n"
                    "        <h4>Fluxo de agua</h4><p><span>%.2f L/min</span></p>\n"
                    "      </div>\n"
                    "    </div>\n"
                    "  </div>\n"
                    "</body>\n"
                    "</html>";

int map(int value, int in_min, int in_max, int out_min, int out_max) {
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void readings(){
    vTaskResume(task_handle_contador);
    vTaskResume(task_handle_temperatura);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    if (event_id == WIFI_EVENT_STA_START){
        ESP_LOGI(TAG, "Conectando ...");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Conectado a rede local");
        wifi_connect_status = 1;
        xEventGroupSetBits(xStatusRede_handle_eventGroup, ESTADO_WIFI);
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Desconectado da rede local");
        wifi_connect_status = 0;
        xEventGroupClearBits(xStatusRede_handle_eventGroup, ESTADO_WIFI);
        if (retry_num < RETRY_NUM) {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Reconectando ...");
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP: " IPSTR,IP2STR(&event->ip_info.ip));
        retry_num = 0;
    }
}


void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SSID:%s  password:%s",SSID,PASSWORD);
}

esp_err_t web_page(httpd_req_t *req){
    float temperatura = 0;
    float calculo = 0;

    readings();
    char response_data[sizeof(html_page) + 50];
    memset(response_data, 0x00, sizeof(response_data));
    
    xQueueReceive(queue_handle_temperatura_e_calc, &temperatura, portMAX_DELAY);
    xQueueReceive(queue_handle_temperatura_e_calc, &calculo, portMAX_DELAY);
    sprintf(response_data, html_page, temperatura, calculo);
    return httpd_resp_send(req, response_data, HTTPD_RESP_USE_STRLEN);
}

esp_err_t req_handler(httpd_req_t *req){
    return web_page(req);
}

httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET, // Método de requisição
    .handler = req_handler, // Handler 
    .user_ctx = NULL // Pointeiro usado para receber o contexto (dados) oferecido pelo handler  
};

httpd_handle_t setup_server(void){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK){
        httpd_register_uri_handler(server, &uri_get); 
    }

    return server;
}

void task_temperatura(void *arg){
    int16_t temp;
    while(1){
        if(dht_read_data(DHT_TYPE_AM2301, DHT_PIN, NULL, &temp) != ESP_OK){
            ESP_LOGI(TAG, "Não foi possivel fazer leitura");
            vTaskSuspend(task_handle_temperatura);
        }else{
            ESP_LOGI(TAG, "Temperatura: %.2f C", (float)temp/10);
        }

        float t = (float) temp/10;
        xQueueSend(queue_handle_temperatura_e_calc, (void *) &t, 0);

        vTaskSuspend(task_handle_temperatura);
    }
    vTaskDelete(NULL);
}

void task_contador(void *args){
    uint32_t valor;
    uint32_t contador = 0;
    float calculo = 0;
    while(1){
        contador = 0;
        
        valor = adc1_get_raw(PIN_POT);
        contador = map(valor, 0, 4096 ,0 , 50);

        ESP_LOGW(TAG, "%u ondas por segundo", contador);
        calculo = (float) contador * 2.25;
        calculo = calculo *60;
        calculo = calculo /1000;
        ESP_LOGI(TAG, "%.2f L/minutos", calculo);

        vTaskDelay(pdMS_TO_TICKS(1000));
        xQueueSend(queue_handle_temperatura_e_calc, (void *) &calculo, 0);
        vTaskSuspend(task_handle_contador);
    }
}

void tarefa_eventGroup(void *arg){
    EventBits_t evento;
    uint32_t status_led = 0;
    while(1){
        evento = xEventGroupWaitBits(xStatusRede_handle_eventGroup, ESTADO_WIFI, pdFALSE, pdTRUE, pdMS_TO_TICKS(100));
        //printf("ss %08x\n", evento);

        if(evento & ESTADO_WIFI){
            if(!status_led){
                status_led = 1;
                gpio_set_level(LED_BULTIN, 1);
            }
        }else{
            if(status_led){
                status_led = 0;
                gpio_set_level(LED_BULTIN, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
    vTaskDelete(NULL);
}

void app_main() {
    init_hardware();

    xStatusRede_handle_eventGroup = xEventGroupCreate();
    if(xStatusRede_handle_eventGroup == NULL){
        ESP_LOGE(TAG, "Erro ao criar o EventGroup.");
        return;
    }

    queue_handle_temperatura_e_calc = xQueueCreate(2, sizeof(float));
    if(queue_handle_temperatura_e_calc == NULL){
        ESP_LOGE(TAG, "Erro ao criar o queue responsável pelo envio e recebimento de dados da temperatura e do calculo do fluxo da agua.");
        return;
    }

   
    xTaskCreate(task_contador, "task_contador" , 2048, NULL, 2, &task_handle_contador);
    xTaskCreate(task_temperatura, "task_temperatura" , 2048, NULL, 2, &task_handle_temperatura);
    xTaskCreate(tarefa_eventGroup, "Escutando eventos" , 2048, NULL, 2, &task_handle_contador);

    esp_err_t ret = nvs_flash_init ();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init ();
    
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (wifi_connect_status) {
        setup_server();
        ESP_LOGI(TAG, "Web Server inicializado");
    } else {
        ESP_LOGI(TAG, "Falha ao conectar à rede local");
    }
}