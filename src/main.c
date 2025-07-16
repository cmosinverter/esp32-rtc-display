#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_http_client.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/apps/sntp.h"
#include "cJSON.h"
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

#define WIFI_SSID "ASTRON_BIO"
#define WIFI_PASS "astron808"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *WIFI_TAG = "WIFI";
static const char *NTP_TAG = "NTP";

static int s_retry_num = 0;

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


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


static void wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
    }
}

static void obtain_time(uint8_t *time_buf) {
    ESP_LOGI(NTP_TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.google.com");  // You can use "time.google.com" too
    sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2024 - 1900) && ++retry < retry_count) {
        ESP_LOGI(NTP_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    time_buf[0] = (uint8_t)((timeinfo.tm_sec % 10) | ((timeinfo.tm_sec / 10) << 4));
    time_buf[1] = (uint8_t)((timeinfo.tm_min % 10) | ((timeinfo.tm_min / 10) << 4));
    time_buf[2] = (uint8_t)((timeinfo.tm_hour % 10) | ((timeinfo.tm_hour / 10) << 4));
    time_buf[3] = (uint8_t)((timeinfo.tm_wday + 1) & 0x07); // Day of week (1=Monday)
    time_buf[4] = (uint8_t)((timeinfo.tm_mday % 10) | ((timeinfo.tm_mday / 10) << 4));
    time_buf[5] = (uint8_t)((timeinfo.tm_mon + 1) % 10) | ((timeinfo.tm_mon / 10) << 4); // Month (1-12)
    time_buf[6] = (uint8_t)((timeinfo.tm_year - 100) % 10) | (((timeinfo.tm_year - 100) / 10) << 4); // Year (00-99)

    if (timeinfo.tm_year >= (2024 - 1900)) {
        ESP_LOGI(NTP_TAG, "Time is set");
    } else {
        ESP_LOGE(NTP_TAG, "Failed to get NTP time");
    }
    
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

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    setenv("TZ", "CST-8", 1);
    tzset();
    uint8_t time_data[7];
    obtain_time(time_data);

    printf("Starting DS3231 RTC + VK16K33C Display\n");

    if (i2c_init() != ESP_OK) {
        printf("I2C initialization failed!\n");
        return;
    }

    if (ds3231_init(ds3231_dev_handle, time_data) != ESP_OK) {
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
