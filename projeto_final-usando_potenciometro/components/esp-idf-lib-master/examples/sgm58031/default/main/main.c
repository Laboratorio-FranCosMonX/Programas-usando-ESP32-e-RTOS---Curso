#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include <sgm58031.h>

#define I2C_PORT 0

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#define GAIN SGM58031_GAIN_2V048 // +-2.048V

static const char *TAG = "example";
// I2C addresses
static const uint8_t addr = SGM58031_ADDR_GND;

// Descriptors
static i2c_dev_t device;

// Gain value
static float gain_val;

// Main task
void sgm58031_test(void *pvParameters)
{
    gain_val = sgm58031_gain_values[GAIN];

    // Setup ICs

    ESP_ERROR_CHECK(
        sgm58031_init_desc(&device, addr, I2C_PORT, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));

    uint8_t id, version;
    ESP_ERROR_CHECK(sgm58031_get_chip_id(&device, &id, &version));
    ESP_LOGI(TAG, "Device - Addr 0x%02x\tID: %d\tVersion: %d", addr, id, version);

    ESP_ERROR_CHECK(sgm58031_set_data_rate(&device, SGM58031_DATA_RATE_240)); // 25 samples per second
    ESP_ERROR_CHECK(sgm58031_set_input_mux(&device, SGM58031_MUX_AIN0_GND));  // positive = AIN0, negative = GND
    ESP_ERROR_CHECK(sgm58031_set_gain(&device, GAIN));
    ESP_ERROR_CHECK(sgm58031_set_conv_mode(&device, SGM58031_CONV_MODE_CONTINUOUS)); // Continuous conversion mode

    while (1)
    {
        // Read result
        int16_t raw = 0;
        if (sgm58031_get_value(&device, &raw) == ESP_OK)
        {
            float voltage = gain_val / SGM58031_MAX_VALUE * raw;
            ESP_LOGI(TAG, "[0x%02x] Raw ADC value: %d, voltage: %.04f volts", addr, raw, voltage);
        }
        else
        {
            ESP_LOGE(TAG, "[0x%02x] Cannot read ADC value", addr);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main()
{
    // Init library
    ESP_ERROR_CHECK(i2cdev_init());

    // Clear device descriptors
    memset(&device, 0, sizeof(device));

    // Start task
    xTaskCreatePinnedToCore(sgm58031_test, "sgm58031_test", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);
}
