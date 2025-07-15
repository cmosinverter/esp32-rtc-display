#include "ds3231.h"

esp_err_t ds3231_init(i2c_master_dev_handle_t dev_handle) {
    uint8_t current_time_cmd[8] = {
        0x00, // Register address to write to
        0x00, // seconds
        0x53, // minutes
        0x17, // hours
        0x02, // day of week (1=Monday)
        0x13, // date
        0x05, // month
        0x14  // year (20xx, e.g., 0x14 for 2014)
    };

    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, current_time_cmd, 8, 1000));
    return ESP_OK;
}

esp_err_t ds3231_read_current_time(i2c_master_dev_handle_t dev_handle, uint8_t *time_type, uint8_t *time_data) {
    return i2c_master_transmit_receive(dev_handle, time_type, 1, time_data, 7, 1000);
}