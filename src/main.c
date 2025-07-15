#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "ds3231.h"
#include "vk16k33c.h"

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#define I2C_PORT_0 I2C_NUM_0
#define SCL_IO_PORT 22
#define SDA_IO_PORT 21
#define I2C_CLK_SPEED_HZ 100000

static const int led_pin = 2;

// I2C handles
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t ds3231_dev_handle;
static i2c_master_dev_handle_t vk16k33c_dev_handle;

esp_err_t i2c_init() {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT_0,
        .scl_io_num = SCL_IO_PORT,
        .sda_io_num = SDA_IO_PORT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t ds3231_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_ADDR,
        .scl_speed_hz = I2C_CLK_SPEED_HZ,
    };

    i2c_device_config_t vk16k33c_dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VK16K33C_ADDR,
        .scl_speed_hz = I2C_CLK_SPEED_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ds3231_dev_cfg, &ds3231_dev_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &vk16k33c_dev_cfg, &vk16k33c_dev_handle));

    printf("I2C initialized successfully\n");
    return ESP_OK;
}


void blink_task(void *pvParameter) {
    gpio_reset_pin(led_pin);
    gpio_set_direction(led_pin, GPIO_MODE_OUTPUT);
    
    while (1) {
        gpio_set_level(led_pin, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(led_pin, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void rtc_task(void *pvParameter) {

    uint8_t time_data[7];
    uint8_t reg_addr = 0x00;
    bool colon_on = false;

    while (1) {
        if (ds3231_read_current_time(ds3231_dev_handle, &reg_addr, time_data) == ESP_OK) {
            printf("Current Time: 20%02X-%02X-%02X %02X:%02X:%02X\n",
                   time_data[6], time_data[5], time_data[4],
                   time_data[2], time_data[1], time_data[0]);

            // Convert BCD to decimal
            uint8_t hour = ((time_data[2] >> 4) * 10) + (time_data[2] & 0x0F);
            uint8_t minute = ((time_data[1] >> 4) * 10) + (time_data[1] & 0x0F);
            
            vk16k33c_display_time(vk16k33c_dev_handle, hour, minute, colon_on);
            colon_on = !colon_on;  // Toggle colon state for next display

        } else {
            printf("Failed to read RTC time\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main() {

    printf("Starting DS3231 RTC + VK16K33C Display\n");

    if (i2c_init() != ESP_OK) {
        printf("I2C initialization failed!\n");
        return;
    }

    if (ds3231_init(ds3231_dev_handle) != ESP_OK) {
        printf("DS3231 initialization failed!\n");
        return;
    }

    if (vk16k33c_init(vk16k33c_dev_handle) != ESP_OK) {
        printf("VK16K33C initialization failed!\n");
        return;
    }

    xTaskCreatePinnedToCore(blink_task, "blink_task", 2048, NULL, 5, NULL, app_cpu);
    xTaskCreatePinnedToCore(rtc_task, "rtc_task", 4096, NULL, 6, NULL, app_cpu);
}
