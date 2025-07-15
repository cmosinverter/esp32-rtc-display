#ifndef VK16K33C_H
#define VK16K33C_H

#include "driver/i2c_master.h"

// VK16K33C I2C address
#define VK16K33C_ADDR 0x70

static const uint8_t digit_to_segment[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

esp_err_t vk16k33c_init(i2c_master_dev_handle_t dev_handle);
esp_err_t vk16k33c_display_time(i2c_master_dev_handle_t dev_handle, uint8_t hour, uint8_t minute, bool colon_on);

#endif // VK1633C_H