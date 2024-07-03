#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/message_buffer.h"
#include "driver/gpio.h"
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

#define SSID "Gato Preto"
#define PASSWORD "89324312"
#define ESP_MAXIMUM_RETRY 5

#define TAG "Aula 12 - Projeto final"
#define RETRY_NUM 5
int wifi_connect_status = 0;
int retry_num = 0;

#define BUTTON_PIN 4
#define LED_PIN 2
#define DHT_PIN 23

volatile uint32_t debounce = 0;
MessageBufferHandle_t message_handle;
QueueHandle_t queue_handle_temperatura_e_calc;
QueueHandle_t queue_handle_calculo;
TaskHandle_t task_handle_contador;

void IRAM_ATTR button_isr_handler(void* arg) {
    debounce++;
    if( debounce > 2){
        uint32_t contador = 1;
        xMessageBufferSendFromISR(message_handle, (void *) &contador, sizeof(contador), NULL);
        debounce = 0;
    }
}

void init_hardware(){
    // Configuração do pino de botão como entrada
    gpio_config_t button_config = {
        .pin_bit_mask = (1 << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_NEGEDGE // Interrupção de borda de descida
    };
    gpio_config(&button_config);

    // Configuração do pino do LED como saída
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
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
                    "        <h4>Fluxo de agua</h4><p><span>%.2f ml/min</span></p>\n"
                    "      </div>\n"
                    "    </div>\n"
                    "  </div>\n"
                    "</body>\n"
                    "</html>";

void readings(){
    int16_t temp;
    if(dht_read_data(DHT_TYPE_AM2301, DHT_PIN, NULL, &temp) != ESP_OK){
        ESP_LOGI(TAG, "Não foi possivel fazer leitura");
    }

    gpio_intr_enable(BUTTON_PIN);
    vTaskResume(task_handle_contador);
    vTaskDelay(pdMS_TO_TICKS(2100));

    float t = (float) temp/10;
    float calc;
    if(!xQueueReceive(queue_handle_calculo, &calc, 0)){
        ESP_LOGE(TAG, "Nao houve o recebimento do cálculo.");
    }else{
        xQueueSend(queue_handle_temperatura_e_calc, (void *) &t, 0);
        xQueueSend(queue_handle_temperatura_e_calc, (void *) &calc, 0);
    }
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data){
    if (event_id == WIFI_EVENT_STA_START){
        ESP_LOGI(TAG, "Conectando ...");
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "Conectado a rede local");
        wifi_connect_status = 1;
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Desconectado da rede local");
        wifi_connect_status = 0;
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

void task_contador(void *args){
    uint32_t va;
    uint32_t contador = 0;
    float onda = 0;
    while(1){
        contador = 0;
        int i;
        gpio_intr_enable(BUTTON_PIN);
        for (i = 0 ; i < 100; i++) {
            if(xMessageBufferReceive(message_handle, &va, sizeof(message_handle), 0)){
                contador = contador + va;
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        gpio_intr_disable(BUTTON_PIN);

        ESP_LOGI(TAG, "O botao foi pressionado %u vezes", contador);
        onda = (float) contador * 2.25;
        onda = onda *60;
        onda = onda /1000;
        ESP_LOGI(TAG, "%.2f ml/minutos\n", onda);

        xQueueSend(queue_handle_calculo, (void *) &onda, 50);
        vTaskSuspend(task_handle_contador);
    }
}

void app_main() {
    init_hardware();

    // Configuração do tratador de interrupção do botão
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);

    message_handle = xMessageBufferCreate(50);
    if(message_handle == NULL){
        //error
    }

    queue_handle_temperatura_e_calc = xQueueCreate(2, sizeof(float));
    if(queue_handle_temperatura_e_calc == NULL){
        //error
    }

    queue_handle_calculo = xQueueCreate(1, sizeof(float));
    if(queue_handle_calculo == NULL){
        //error
    }

    xTaskCreate(task_contador, "task_contador" , 2048, NULL, 2, &task_handle_contador);

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