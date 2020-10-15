# stm32f405rg on-chip flash demo

## What

KVDB  and TSDB demo on STM32F4X chip

- MCU: STM32F405RG
- Flash Driver
  - STM32 on-chip flash
- IO
  - UART
    - TXD: PA9
    - RXD: PA10

## How

### Step1: connect the serial terminal

Connect the board to PC by serial port and open the PC serial terminal.

### Step2: open demo project

support 2 kinds of IDEs

- Keil MDK: open `RVMDK\FlashDB.uvprojx`
- [RT-Thread Studio](https://www.rt-thread.io/studio.html): import this folder by import wizard

### Step3: build and download

Download the firmware to your board when build successful.

### Step4: check the log

This demo's log will output to PC serial.

