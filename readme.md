# FreeRTOS ESP32 RTC Display Project

A FreeRTOS-based ESP32 project that interfaces with a DS3231 Real-Time Clock (RTC) module and displays the time on a VK16K33C-based 7-segment display.

## Features

- **Real-Time Clock**: DS3231 RTC module for accurate timekeeping
- **7-Segment Display**: VK16K33C controller for time display
- **LED Blinking**: Visual indicator on GPIO pin 2
- **I2C Communication**: Manages multiple I2C devices on a single bus
- **FreeRTOS Tasks**: Multi-threaded operation with separate tasks for different functions
- **Time Display**: Shows current time in HH:MM format with blinking colon

## Hardware Requirements

- ESP32 development board (ESP32-DOIT-DEVKIT-V1 or compatible)
- DS3231 RTC module
- VK16K33C-based 7-segment display (4-digit)
- Connecting wires
- Breadboard (optional)

## Pin Configuration

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| DS3231 SDA | GPIO 21 | I2C Data Line |
| DS3231 SCL | GPIO 22 | I2C Clock Line |
| VK16K33C SDA | GPIO 21 | I2C Data Line (shared) |
| VK16K33C SCL | GPIO 22 | I2C Clock Line (shared) |
| LED | GPIO 2 | Status LED |

## I2C Addresses

- **DS3231 RTC**: 0x68
- **VK16K33C Display**: 0x70

## Project Structure

```
freertos-learn/
├── include/               # Header files
│   └── README
├── lib/                   # Library source files
│   ├── ds3231/           # DS3231 RTC driver
│   │   ├── ds3231.c
│   │   └── ds3231.h
│   └── vk16k33c/         # VK16K33C display driver
│       ├── vk16k33c.c
│       └── vk16k33c.h
├── src/                   # Main application
│   ├── main.c            # Main program
│   └── CMakeLists.txt
├── test/                  # Test files
├── CMakeLists.txt         # Main CMake configuration
├── platformio.ini         # PlatformIO configuration
└── readme.md             # This file
```

## How It Works

### Tasks

1. **Blink Task**: 
   - Toggles the onboard LED every 500ms
   - Provides visual indication that the system is running
   - Priority: 5

2. **RTC Task**:
   - Reads current time from DS3231 every second
   - Converts BCD format to decimal
   - Displays time on VK16K33C 7-segment display
   - Toggles colon separator for visual indication
   - Priority: 6

### I2C Configuration

- **Clock Speed**: 100 kHz
- **Port**: I2C_NUM_0
- **Pull-up resistors**: Internal pull-ups enabled
- **Glitch Filter**: 7 clock cycles

## Building and Flashing

### Using ESP-IDF

```bash
# Configure the project
idf.py menuconfig

# Build the project
idf.py build

# Flash to ESP32
idf.py flash

# Monitor serial output
idf.py monitor
```

### Using PlatformIO

```bash
# Build the project
pio run

# Flash to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Serial Output

The project outputs debug information via UART:

```
Starting DS3231 RTC + VK16K33C Display
I2C initialized successfully
DS3231 initialization failed!
Current Time: 2024-05-13 17:53:00
Current Time: 2024-05-13 17:53:01
...
```

## Configuration

### Setting Initial Time

The DS3231 is initialized with a default time in the `ds3231_init()` function. To set a different initial time, modify the `current_time_cmd` array in the DS3231 initialization:

```c
uint8_t current_time_cmd[8] = {
    0x00, // Register address
    0x00, // seconds (BCD)
    0x53, // minutes (BCD)
    0x17, // hours (BCD)
    0x02, // day of week (1-7)
    0x13, // date (BCD)
    0x05, // month (BCD)
    0x24  // year (BCD, 20xx)
};
```

### Display Configuration

The VK16K33C display brightness and other settings can be configured in the `vk16k33c_init()` function.

## Troubleshooting

### Common Issues

1. **I2C Communication Failed**
   - Check wiring connections
   - Verify I2C addresses
   - Ensure pull-up resistors are connected (or internal ones enabled)

2. **Display Not Working**
   - Verify VK16K33C I2C address
   - Check power supply to display module
   - Ensure proper initialization sequence

3. **RTC Time Reset**
   - DS3231 has a backup battery - check if it needs replacement
   - Verify initial time setting in code

### Debug Tips

- Use `idf.py monitor` to view serial output
- Check I2C bus with logic analyzer if available
- Verify device addresses with I2C scanner code

## Dependencies

- ESP-IDF v4.4 or later
- FreeRTOS (included with ESP-IDF)
- ESP32 I2C Master driver

## License

This project is open source and available under the MIT License.

## Contributing

Feel free to submit issues and pull requests to improve this project.

## Author

Created as a learning project for FreeRTOS and ESP32 I2C communication.