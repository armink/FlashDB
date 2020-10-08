# stm32f405rg demo

## What

KVDB  and TSDB demo on STM32F4X chip

- MCU: STM32F405RG
- Flash Driver
  - spi flash (W25Q64) by using [SFUD](https://github.com/armink/SFUD)
- IO
  - UART
    - TXD: PA9
    - RXD: PA10
  - SPI
    - CS: PB12 (software CS)
    - SCK: PB13
    - MISO: PB14
    - MOSI: PB15

## How

### Step1: connect the serial terminal

Connect the board to PC by serial port and open the PC serial terminal.

### Step2: open demo project

support 2 kinds of IDEs

- Keil MDK: open `RVMDK\FlashDB.uvprojx`
- [RT-Studio](https://www.rt-thread.io/studio.html): import this folder by import wizard

### Step3: build and download

Download the firmware to your board when build successful.

### Step4: check the log

This demo's log will output to PC serial.

