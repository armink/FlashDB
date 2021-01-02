# Based on ESP8266 off-chip SPI-Flash demo

Start a freertos task to run the flashDB routine, and then restart after 1000 seconds. Run log, please see the ESP8266_flashDB_log.rtf log in the current directory

# ESP8266 Development Environment

- Ubuntu 16.04 64-bit
- Use Espressif IDF: ESP8266_RTOS_SDK (version: ESP-IDF v3.2-252-g73bf28b-dirty), download: https://github.com/espressif/ESP8266_RTOS_SDK.git
- Install the cross-compilation tool chain, download: https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz

# ESP8266 Development Guide

- For detailed installation and development tutorials, please refer to: https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html

# Compile and download

- cd into the esp8266_spi_flash directory
- make all
- Use serial cable to connect 8266 development board to PC
- Enable the serial port, execute the command: sudo chmod 777 /dev/ttyUSB*
- make flash
- make monitor