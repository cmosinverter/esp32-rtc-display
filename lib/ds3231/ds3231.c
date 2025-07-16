#include "ds3231.h"

esp_err_t ds3231_init(i2c_master_dev_handle_t dev_handle, uint8_t *time_data) {
    
    uint8_t current_time_cmd[8] = {
        0x00, // Register address to write to
        0x00, // second
        0x34, // minute
        0x02, // hour
        0x03, // day of week (1=Monday)
        0x16, // date
        0x07, // month
        0x25  // year (20xx, e.g., 0x14 for 2014)
    };

    for (int i = 0; i < 7; i++) {
        current_time_cmd[i + 1] = time_data[i];
    }

    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, current_time_cmd, 8, 1000));
    return ESP_OK;
}

esp_err_t ds3231_read_current_time(i2c_master_dev_handle_t dev_handle, uint8_t *time_type, uint8_t *time_data) {
    return i2c_master_transmit_receive(dev_handle, time_type, 1, time_data, 7, 1000);
}