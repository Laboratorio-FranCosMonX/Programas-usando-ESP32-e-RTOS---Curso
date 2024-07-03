#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <esp_http_server.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#include "dht.h"

#define SSID "Gato Preto"
#define PASSWORD "89324312"
#define ESP_MAXIMUM_RETRY 5

#define TAG "Aula 11"
#define RETRY_NUM 5

EventGroupHandle_t xStatusRede_handle_eventGroup;
QueueHandle_t xAcaoBtn_handle_queue;
QueueHandle_t xTempEUmidade_handle_queue;
#define ESTADO_WIFI BIT0
#define LED_BULTIN 2
#define BTN_DISCONECT 4
#define DHT_PIN 23

int wifi_connect_status = 0;
int retry_num = 0;

char html_page[] = "<!DOCTYPE HTML><html>\n"
                   "<head>\n"
                   "  <title>Web Server</title>\n"
                   "  <meta http-equiv=\"refresh\" content=\"10\">\n"
                   "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                   "</head>\n"
                   "<body>\n"
                   "  <div class=\"topnav\">\n"
                   "    <h3>Cursor de Extensao Web Server</h3>\n"
                   "  </div>\n"
                   "  <div class=\"content\">\n"
                   "    <div class=\"cards\">\n"
                   "      <div class=\"card temperatura\">\n"
                   "        <h4>Temperatura</h4><p><span>%.2f&deg;C</span></p>\n"
                   "      </div>\n"
                   "      <div class=\"card umidade\">\n"
                   "        <h4>Umidade</h4><p><span>%.2f</span> &percnt;</span></p>\n"
                   "      </div>\n"
                   "    </div>\n"
                   "  </div>\n"
                   "</body>\n"
                   "</html>";

void init_hardware(){
    gpio_set_direction(BTN_DISCONECT, GPIO_MODE_INPUT);
    //gpio_pullup_en(BTN_DISCONECT);

    gpio_set_direction(LED_BULTIN, GPIO_MODE_OUTPUT);
}

void readings(){
    int16_t temp = 0;
    int16_t hum = 0;

    if(dht_read_data(DHT_TYPE_AM2301, DHT_PIN, &hum, &temp) != ESP_OK){
        ESP_LOGI(TAG, "Não foi possivel fazer leitura");
    }

    xQueueSend(xTempEUmidade_handle_queue, (void *) &temp, 0);
    xQueueSend(xTempEUmidade_handle_queue, (void *) &hum, 0);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    uint8_t resposta_event_btn;
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
        if(xQueueReceive(xAcaoBtn_handle_queue, &resposta_event_btn, portMAX_DELAY) == pdFALSE){
            if (retry_num < RETRY_NUM) {
                esp_wifi_connect();
                retry_num++;
                ESP_LOGI(TAG, "Reconectando ...");
            }
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP: " IPSTR,IP2STR(&event->ip_info.ip));
        retry_num = 0;
    }
}


void wifi_init(void)
{
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
    wifi_config_t wifi_config = 
    {
        .sta = 
        {
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
    int16_t temperatura;
    int16_t umidade;

    readings();
    char response_data[sizeof(html_page) + 50];
    memset(response_data, 0x00, sizeof(response_data));
    
    xQueueReceive(xTempEUmidade_handle_queue, &temperatura, portMAX_DELAY);
    xQueueReceive(xTempEUmidade_handle_queue, &umidade, portMAX_DELAY);
    sprintf(response_data, html_page, (float)temperatura/10, (float)umidade/10);
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

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) // Inicializa o server
    {
        httpd_register_uri_handler(server, &uri_get); // Regitra o handler e a URI
    }

    return server;
}

void tarefa_eventGroup(void *arg){
    EventBits_t evento;
    uint32_t status_led = 0;
    while(1){
        evento = xEventGroupWaitBits(xStatusRede_handle_eventGroup, (ESTADO_WIFI|BTN_DISCONECT), pdFALSE, pdTRUE, pdMS_TO_TICKS(100));
        //printf("ss %08x\n", evento);
        if (evento & BTN_DISCONECT) {
            esp_err_t status_disconect = esp_wifi_disconnect();
            if(status_disconect == ESP_OK){
                ESP_LOGI(TAG, "Wifi desconectado!");
            }else{
                ESP_LOGE(TAG, "Ocorreu um erro ao tentar desconectar.");
            }
        }

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

void tarefa_desconectar(void *args){
    uint8_t debounce = 0;
    uint8_t btn_pressionado = 1;
    while(1){
        if(gpio_get_level(BTN_DISCONECT) == 0){
            debounce++;
            if(debounce >= 2){
                xQueueSend(xAcaoBtn_handle_queue, (void *) &btn_pressionado, 0);
                xEventGroupSetBits(xStatusRede_handle_eventGroup, BTN_DISCONECT);
                debounce = 0;
                vTaskDelay(pdMS_TO_TICKS(100));
                xEventGroupClearBits(xStatusRede_handle_eventGroup, BTN_DISCONECT);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

void app_main(){
    init_hardware();

    xStatusRede_handle_eventGroup = xEventGroupCreate ();
    if (xStatusRede_handle_eventGroup == NULL) {
        ESP_LOGE(TAG, "Erro ao criar o EventGroup.");
        return;
    }
    xAcaoBtn_handle_queue = xQueueCreate(2,sizeof(uint8_t));
    if(xAcaoBtn_handle_queue == NULL){
        ESP_LOGE(TAG, "Erro ao criar o Queue para eventos de btn.");
        return;
    }
    xTempEUmidade_handle_queue = xQueueCreate(2,sizeof(int16_t));
    if(xTempEUmidade_handle_queue == NULL){
        ESP_LOGE(TAG, "Erro ao criar o Queue para comunicação do sensor de temperatura e umidade.");
        return;
    }

    xTaskCreate(tarefa_eventGroup, "tarefa do eventGroup", 1023*3, NULL, 2, NULL);
    xTaskCreate(tarefa_desconectar, "tarefa do botao", 1023*2, NULL, 2, NULL);

    esp_err_t ret = nvs_flash_init ();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init ();
    
    vTaskDelay(pdMS_TO_TICKS(10000));

    if (wifi_connect_status) {
        setup_server();
        ESP_LOGI(TAG, "Web Server inicializado");
    } else {
        ESP_LOGI(TAG, "Falha ao conectar à rede local");
    }
}                  