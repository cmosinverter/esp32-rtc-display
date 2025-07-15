#ifndef DS3231_H
#define DS3231_H

#include "driver/i2c_master.h"

// DS3231 I2C address
#define DS3231_ADDR 0x68

// Function declarations
esp_err_t ds3231_init(i2c_master_dev_handle_t dev_handle);
esp_err_t ds3231_read_current_time(i2c_master_dev_handle_t dev_handle, uint8_t *time_type, uint8_t *time_data);

#endif // DS3231_H