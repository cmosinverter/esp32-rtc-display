#include <vk16k33c.h>

esp_err_t vk16k33c_init(i2c_master_dev_handle_t dev_handle) {
    uint8_t cmd;

    cmd = 0x21;  // System setup: turn on oscillator
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &cmd, 1, 1000));

    cmd = 0x81;  // Display ON, blinking on
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &cmd, 1, 1000));

    cmd = 0xEF;  // Brightness max (15)
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, &cmd, 1, 1000));

    printf("VK16K33C initialized successfully\n");
    return ESP_OK;
}

esp_err_t vk16k33c_display_time(i2c_master_dev_handle_t dev_handle, uint8_t hour, uint8_t minute, bool colon_on) {
    uint8_t display_data[11];
    display_data[0] = 0x00;  // RAM address start

    display_data[1] = digit_to_segment[hour / 10];  // Hour tens
    display_data[2] = 0x00;
    display_data[3] = digit_to_segment[hour % 10];  // Hour units
    display_data[4] = 0x00;

    if (colon_on) {
        display_data[5] = 0xFF;  // colon on
    } else {
        display_data[5] = 0x00;  // colon off
    }
    display_data[6] = 0x00;  // Separator (optional, can be used for colon)

    display_data[7] = digit_to_segment[minute / 10];  // Minute tens
    display_data[8] = 0x00;
    display_data[9] = digit_to_segment[minute % 10];  // Minute units
    display_data[10] = 0x00;

    return i2c_master_transmit(dev_handle, display_data, 11, 1000);
}